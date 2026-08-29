#!/usr/bin/env python3
import sys
import re
import json
from pathlib import Path

def parse_c_value(value_str):
    """Parse C constant values like 0x00011000"""
    value_str = value_str.strip()
    if value_str.startswith('0x') or value_str.startswith('0X'):
        return int(value_str, 16)
    elif value_str.isdigit():
        return int(value_str)
    else:
        raise ValueError(f"Cannot parse C value: {value_str}")

def evaluate(expr, symbols):
    """Evaluate expressions with better whitespace handling"""
    expr = re.sub(r'\s+', ' ', expr.strip())  # Normalize whitespace

    # Pure number (decimal or hex)
    if re.match(r'^(0x[0-9a-fA-F]+|\d+)$', expr):
        return parse_c_value(expr)

    # Identifier already defined
    if expr in symbols:
        return symbols[expr]

    # Parenthesized addition: "(A + B)"
    m = re.match(r'^\(\s*([^+\-]+?)\s*\+\s*([^+\-]+?)\s*\)$', expr)
    if m:
        left, right = m.group(1).strip(), m.group(2).strip()
        left_val = evaluate_operand(left, symbols)
        right_val = evaluate_operand(right, symbols)
        return left_val + right_val

    # Parenthesized subtraction: "(A - B)"
    m = re.match(r'^\(\s*([^+\-]+?)\s*\-\s*([^+\-]+?)\s*\)$', expr)
    if m:
        left, right = m.group(1).strip(), m.group(2).strip()
        left_val = evaluate_operand(left, symbols)
        right_val = evaluate_operand(right, symbols)
        return left_val - right_val

    raise ValueError(f"Unsupported expression: {expr}")

def evaluate_operand(operand, symbols):
    """Evaluate a single operand (number or symbol)"""
    operand = operand.strip()

    # Check if it's a number
    if re.match(r'^(0x[0-9a-fA-F]+|\d+)$', operand):
        return parse_c_value(operand)

    # Check if it's a defined symbol
    if operand in symbols:
        return symbols[operand]

    raise ValueError(f"Undefined symbol: {operand}")

def extract_defines_from_header(filename):
    """Extract #define lines for THIRD_PARTY_EVENT_ID_MIN and EVT_* constants, plus CDU_EVT_OFFSET_* constants"""
    defines = []

    with open(filename, "r", encoding="utf-8") as f:
        for line_num, line in enumerate(f, 1):
            line = line.strip()

            # Look for #define lines
            if line.startswith('#define'):
                # Parse: #define CONSTANT_NAME value_expression
                parts = line.split(None, 2)  # Split into at most 3 parts
                if len(parts) >= 3:
                    define_keyword, const_name, value_expr = parts[0], parts[1], parts[2]

                    # Check if this is a constant we care about
                    if (const_name == 'THIRD_PARTY_EVENT_ID_MIN' or
                        const_name.startswith('EVT_') or
                        const_name.startswith('CDU_EVT_OFFSET_')):
                        # Remove any trailing comment
                        value_expr = re.sub(r'//.*$', '', value_expr).strip()
                        defines.append((const_name, value_expr))

    return defines

def resolve_dependencies(defines):
    """Sort defines to resolve dependencies - simple topological sort"""
    symbols = {}
    unresolved = defines[:]
    resolved_count = 0

    # Keep iterating until we can't resolve any more
    while unresolved and len(unresolved) != resolved_count:
        resolved_count = len(unresolved)
        still_unresolved = []

        for const_name, value_expr in unresolved:
            try:
                value = evaluate(value_expr, symbols)
                symbols[const_name] = value
                yield const_name, value
            except ValueError:
                # Can't resolve yet, try again later
                still_unresolved.append((const_name, value_expr))

        unresolved = still_unresolved

    # Report any that couldn't be resolved
    for const_name, value_expr in unresolved:
        print(f"Error: Could not resolve {const_name} = {value_expr}", file=sys.stderr)

def build_catalog(defines, source, category):
    """Build an msfs-events-style catalog dict from resolved (name, value) pairs"""
    events = [
        {
            "name": const_name,
            "eventId": f"#{value}",
            "params": [],
            "description": ""
        }
        for const_name, value in resolve_dependencies(defines)
    ]
    return {
        "source": source,
        "category": category,
        "events": events
    }

def render_json(defines, source, category):
    catalog = build_catalog(defines, source, category)
    text = json.dumps(catalog, indent=2, ensure_ascii=False) + '\n'
    return text, len(catalog['events'])

def render_txt(defines):
    """Flat 'NAME value' lines - the format part-8's 8-2 example loads via --names"""
    lines = [f"{const_name} {value}" for const_name, value in resolve_dependencies(defines)]
    text = '\n'.join(lines) + ('\n' if lines else '')
    return text, len(lines)

def main():
    import argparse

    parser = argparse.ArgumentParser(description='Extract EVT_* constants from a PMDG SDK header file, as flat "NAME value" text (for 8-2\'s --names loader) or as an msfs-events style JSON catalog')
    parser.add_argument('header_file', help='Path to the header file to process')
    parser.add_argument('-o', '--output', help='Output file (UTF-8, no BOM). If not specified, prints to stdout')
    parser.add_argument('--format', choices=['txt', 'json'], help='Output format. Default: inferred from -o extension (.json -> json, else txt); txt if no -o given')
    parser.add_argument('--source', help='JSON only: value for the catalog "source" field (default: header file stem)')
    parser.add_argument('--category', help='JSON only: value for the catalog "category" field (default: output file stem, or header file stem)')

    args = parser.parse_args()

    header_stem = Path(args.header_file).stem

    fmt = args.format
    if not fmt:
        fmt = 'json' if args.output and Path(args.output).suffix.lower() == '.json' else 'txt'

    try:
        defines = extract_defines_from_header(args.header_file)

        if fmt == 'json':
            source = args.source or header_stem
            category = args.category or (Path(args.output).stem if args.output else header_stem)
            text, count = render_json(defines, source, category)
        else:
            text, count = render_txt(defines)

        if args.output:
            with open(args.output, 'w', encoding='utf-8', newline='\n') as f:
                f.write(text)
            print(f"Output written to {args.output} ({count} events)", file=sys.stderr)
        else:
            print(text, end='')

    except Exception as e:
        print(f"Error processing file {args.header_file}: {e}", file=sys.stderr)
        sys.exit(1)

if __name__ == "__main__":
    main()
