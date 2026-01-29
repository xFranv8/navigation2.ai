#!/usr/bin/env python3
# Copyright 2026 xfranv8
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

"""
Convert Depth Anything V2 PyTorch model (.pth) to ONNX format.

This script converts the Depth Anything V2 model weights to ONNX format
for efficient inference with ONNX Runtime.

Usage:
    python3 convert_pth_to_onnx.py --input model.pth --output model.onnx

Requirements:
    pip install torch torchvision onnx
    # Clone Depth Anything V2 repository for model definition
    git clone https://github.com/DepthAnything/Depth-Anything-V2.git
"""

import argparse
from pathlib import Path
import sys


def parse_args():
    parser = argparse.ArgumentParser(
        description='Convert Depth Anything V2 .pth model to ONNX format'
    )
    parser.add_argument(
        '--input', '-i',
        type=str,
        required=True,
        help='Path to input .pth model file'
    )
    parser.add_argument(
        '--output', '-o',
        type=str,
        default=None,
        help='Path to output .onnx file (default: same name with .onnx extension)'
    )
    parser.add_argument(
        '--encoder',
        type=str,
        default='vits',
        choices=['vits', 'vitb', 'vitl', 'vitg'],
        help='Encoder type: vits (small), vitb (base), vitl (large), vitg (giant)'
    )
    parser.add_argument(
        '--input-size',
        type=int,
        default=518,
        help='Input image size (default: 518)'
    )
    parser.add_argument(
        '--opset',
        type=int,
        default=17,
        help='ONNX opset version (default: 17)'
    )
    parser.add_argument(
        '--depth-anything-path',
        type=str,
        default=None,
        help='Path to Depth-Anything-V2 repository (if not in PYTHONPATH)'
    )
    return parser.parse_args()


def main():
    args = parse_args()

    # Validate input file
    input_path = Path(args.input)
    if not input_path.exists():
        print(f'Error: Input file not found: {input_path}')
        sys.exit(1)

    # Set output path
    if args.output is None:
        output_path = input_path.with_suffix('.onnx')
    else:
        output_path = Path(args.output)

    print(f'Converting: {input_path} -> {output_path}')
    print(f'Encoder: {args.encoder}, Input size: {args.input_size}')

    # Add Depth Anything path if provided
    if args.depth_anything_path:
        sys.path.insert(0, args.depth_anything_path)

    try:
        import torch
        import onnx
    except ImportError as e:
        print(f'Error: Missing required package: {e}')
        print('Please install: pip install torch onnx')
        sys.exit(1)

    # Try to import Depth Anything V2 model
    try:
        from depth_anything_v2.dpt import DepthAnythingV2
    except ImportError:
        print('Error: Cannot import Depth Anything V2 model definition.')
        print('Please either:')
        print('  1. Clone the repo: '
              'git clone https://github.com/DepthAnything/Depth-Anything-V2.git')
        print('  2. Use --depth-anything-path to specify the repo location')
        sys.exit(1)

    # Model configurations
    model_configs = {
        'vits': {'encoder': 'vits', 'features': 64, 'out_channels': [48, 96, 192, 384]},
        'vitb': {'encoder': 'vitb', 'features': 128, 'out_channels': [96, 192, 384, 768]},
        'vitl': {'encoder': 'vitl', 'features': 256, 'out_channels': [256, 512, 1024, 1024]},
        'vitg': {'encoder': 'vitg', 'features': 384, 'out_channels': [1536, 1536, 1536, 1536]},
    }

    config = model_configs[args.encoder]

    # Create model
    print(f'Creating model with config: {config}')
    model = DepthAnythingV2(**config)

    # Load weights
    print('Loading weights...')
    state_dict = torch.load(input_path, map_location='cpu')
    model.load_state_dict(state_dict)
    model.eval()

    # Create dummy input
    input_size = args.input_size
    dummy_input = torch.randn(1, 3, input_size, input_size)

    # Export to ONNX using legacy exporter for compatibility with onnxruntime 1.16.x
    # (supports ONNX IR version up to 9)
    print('Exporting to ONNX...')
    torch.onnx.export(
        model,
        dummy_input,
        str(output_path),
        export_params=True,
        opset_version=args.opset,
        do_constant_folding=True,
        input_names=['input'],
        output_names=['output'],
        dynamic_axes={
            'input': {0: 'batch_size', 2: 'height', 3: 'width'},
            'output': {0: 'batch_size', 2: 'height', 3: 'width'}
        },
        dynamo=False  # Use legacy exporter for IR version 8 compatibility
    )

    # Verify the model
    print('Verifying ONNX model...')
    onnx_model = onnx.load(str(output_path))
    onnx.checker.check_model(onnx_model)

    print(f'Successfully converted to: {output_path}')
    print(f'Model size: {output_path.stat().st_size / 1024 / 1024:.2f} MB')


if __name__ == '__main__':
    main()
