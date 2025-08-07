#!/usr/bin/env python3
"""
Code validation script for ESP32 RFID Audio Recorder
Performs basic syntax and structure checks
"""

import re
import os

def validate_arduino_sketch(filename):
    """Validate basic Arduino sketch structure and syntax"""
    
    with open(filename, 'r') as f:
        content = f.read()
    
    errors = []
    warnings = []
    
    # Check for required functions
    required_functions = ['setup()', 'loop()']
    for func in required_functions:
        if func not in content:
            errors.append(f"Missing required function: {func}")
    
    # Check for balanced braces
    open_braces = content.count('{')
    close_braces = content.count('}')
    if open_braces != close_braces:
        errors.append(f"Unbalanced braces: {open_braces} opening, {close_braces} closing")
    
    # Check for balanced parentheses in function calls
    open_parens = content.count('(')
    close_parens = content.count(')')
    if open_parens != close_parens:
        errors.append(f"Unbalanced parentheses: {open_parens} opening, {close_parens} closing")
    
    # Check for proper includes
    required_includes = ['#include <SPI.h>', '#include <MFRC522.h>', '#include <SD.h>']
    for include in required_includes:
        if include not in content:
            warnings.append(f"Missing recommended include: {include}")
    
    # Check for pin definitions in both main file and config.h
    config_content = ""
    if os.path.exists("config.h"):
        with open("config.h", 'r') as f:
            config_content = f.read()
    
    combined_content = content + config_content
    pin_patterns = [
        r'#define\s+\w+_PIN\s+\d+',
        r'#define\s+I2S_\w+\s+\d+'
    ]
    
    pin_definitions_found = 0
    for pattern in pin_patterns:
        pin_definitions_found += len(re.findall(pattern, combined_content))
    
    if pin_definitions_found < 8:  # Expecting at least 8 pin definitions
        warnings.append(f"Few pin definitions found: {pin_definitions_found}")
    
    # Check for state machine implementation
    if 'enum SystemState' not in content:
        warnings.append("State machine enum not found")
    
    if 'updateStateMachine()' not in content:
        warnings.append("State machine update function not found")
    
    # Check for button debouncing
    if 'BUTTON_DEBOUNCE_DELAY' not in content:
        warnings.append("Button debouncing constant not found")
    
    # Check for RFID timing improvements
    if 'RFID_REMOVAL_DELAY' not in content:
        warnings.append("RFID removal delay constant not found")
    
    # Check for proper variable declarations
    global_vars = [
        'currentState', 'rfidTagPresent', 'buttonPressed', 
        'lastDebounceTime', 'rfidRemovalStartTime'
    ]
    
    for var in global_vars:
        if var not in content:
            warnings.append(f"Expected global variable not found: {var}")
    
    return errors, warnings

def validate_code_quality(filename):
    """Check code quality aspects"""
    
    with open(filename, 'r') as f:
        lines = f.readlines()
    
    quality_issues = []
    
    # Check for excessive line length
    for i, line in enumerate(lines, 1):
        if len(line.strip()) > 120:
            quality_issues.append(f"Line {i}: Line too long ({len(line.strip())} chars)")
    
    # Check for proper commenting
    comment_lines = sum(1 for line in lines if line.strip().startswith('//') or line.strip().startswith('/*'))
    total_lines = len([line for line in lines if line.strip()])
    comment_ratio = comment_lines / total_lines if total_lines > 0 else 0
    
    if comment_ratio < 0.1:  # Less than 10% comments
        quality_issues.append(f"Low comment ratio: {comment_ratio:.2%}")
    
    return quality_issues

def main():
    sketch_file = 'ESP32_RFID_Audio_Recorder.ino'
    
    if not os.path.exists(sketch_file):
        print(f"Error: {sketch_file} not found")
        return 1
    
    print("Validating ESP32 RFID Audio Recorder code...")
    print("=" * 50)
    
    # Validate basic structure
    errors, warnings = validate_arduino_sketch(sketch_file)
    
    # Check code quality
    quality_issues = validate_code_quality(sketch_file)
    
    # Report results
    if errors:
        print("ERRORS:")
        for error in errors:
            print(f"  ❌ {error}")
        print()
    
    if warnings:
        print("WARNINGS:")
        for warning in warnings:
            print(f"  ⚠️  {warning}")
        print()
    
    if quality_issues:
        print("CODE QUALITY ISSUES:")
        for issue in quality_issues:
            print(f"  📝 {issue}")
        print()
    
    # Summary
    total_issues = len(errors) + len(warnings) + len(quality_issues)
    
    if errors:
        print("❌ VALIDATION FAILED - Code has errors that must be fixed")
        return 1
    elif warnings or quality_issues:
        print(f"⚠️  VALIDATION PASSED WITH WARNINGS - {total_issues} issues found")
        return 0
    else:
        print("✅ VALIDATION PASSED - No issues found")
        return 0

if __name__ == '__main__':
    exit(main())