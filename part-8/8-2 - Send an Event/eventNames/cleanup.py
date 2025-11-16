#!/usr/bin/env python3
import sys
import re

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

def main():
    import argparse
    
    parser = argparse.ArgumentParser(description='Extract and evaluate EVT_* constants from PMDG SDK header files')
    parser.add_argument('header_file', help='Path to the header file to process')
    parser.add_argument('-o', '--output', help='Output file (UTF-8, no BOM). If not specified, prints to stdout')
    
    args = parser.parse_args()
    
    try:
        # Extract relevant #define lines from the header file
        defines = extract_defines_from_header(args.header_file)
        
        # Resolve dependencies and process in correct order
        output_lines = []
        for const_name, value in resolve_dependencies(defines):
            output_lines.append(f"{const_name} {value}")
        
        # Write to file or stdout
        if args.output:
            with open(args.output, 'w', encoding='utf-8') as f:
                for line in output_lines:
                    f.write(line + '\n')
            print(f"Output written to {args.output} ({len(output_lines)} constants)", file=sys.stderr)
        else:
            for line in output_lines:
                print(line)
                
    except Exception as e:
        print(f"Error processing file {args.header_file}: {e}", file=sys.stderr)
        sys.exit(1)

if __name__ == "__main__":
    main()
