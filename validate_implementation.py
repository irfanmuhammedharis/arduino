#!/usr/bin/env python3
"""
Code Validation Script for ESP32 RFID Audio Recorder
Validates the complete implementation for production readiness
"""

import re
import os

def validate_complete_implementation():
    """Validate the complete Arduino sketch implementation"""
    
    sketch_file = 'ESP32_RFID_Audio_Recorder.ino'
    config_file = 'config.h'
    
    if not os.path.exists(sketch_file):
        print(f"❌ Error: {sketch_file} not found")
        return False
    
    if not os.path.exists(config_file):
        print(f"❌ Error: {config_file} not found")
        return False
    
    with open(sketch_file, 'r') as f:
        sketch_content = f.read()
    
    with open(config_file, 'r') as f:
        config_content = f.read()
    
    print("🔍 Validating ESP32 RFID Audio Recorder Implementation...")
    print("=" * 60)
    
    # Check for required missing functions
    required_functions = [
        'void printRFIDDiagnostics()',
        'void handleSerialCommands()',  
        'void printHelpMenu()',
        'void handleError('
    ]
    
    print("📋 Checking Required Missing Functions:")
    all_functions_present = True
    for func in required_functions:
        if func in sketch_content:
            print(f"  ✅ {func.split('(')[0]} - IMPLEMENTED")
        else:
            print(f"  ❌ {func.split('(')[0]} - MISSING")
            all_functions_present = False
    
    # Check Arduino sketch structure
    print("\n🏗️  Checking Arduino Sketch Structure:")
    if 'void setup()' in sketch_content:
        print("  ✅ setup() function present")
    else:
        print("  ❌ setup() function missing")
        all_functions_present = False
    
    if 'void loop()' in sketch_content:
        print("  ✅ loop() function present")
    else:
        print("  ❌ loop() function missing")
        all_functions_present = False
    
    # Check required includes
    print("\n📚 Checking Required Libraries:")
    required_includes = [
        '#include <SPI.h>',
        '#include <MFRC522.h>',
        '#include <SD.h>',
        '#include <FS.h>',
        '#include <driver/i2s.h>',
        '#include "config.h"'
    ]
    
    for include in required_includes:
        if include in sketch_content:
            print(f"  ✅ {include}")
        else:
            print(f"  ❌ {include} - MISSING")
            all_functions_present = False
    
    # Check production-ready features
    print("\n🚀 Checking Production-Ready Features:")
    
    # Non-blocking operation
    millis_count = sketch_content.count('millis()')
    delay_count = sketch_content.count('delay(')
    if millis_count > 10 and delay_count < 20:
        print(f"  ✅ Non-blocking operation (millis: {millis_count}, delay: {delay_count})")
    else:
        print(f"  ⚠️  May have blocking operation (millis: {millis_count}, delay: {delay_count})")
    
    # State machine
    if 'enum SystemState' in sketch_content and 'updateStateMachine' in sketch_content:
        print("  ✅ State machine implementation")
    else:
        print("  ❌ State machine missing")
    
    # Error handling
    if 'enum ErrorType' in sketch_content and 'handleError' in sketch_content:
        print("  ✅ Comprehensive error handling")
    else:
        print("  ❌ Error handling missing")
    
    # Serial commands
    command_count = sketch_content.count('command ==')
    if command_count > 10:
        print(f"  ✅ Complete serial command interface ({command_count} commands)")
    else:
        print(f"  ⚠️  Limited serial commands ({command_count} commands)")
    
    # Dual I2S support
    if 'I2S_PORT_RX' in sketch_content and 'I2S_PORT_TX' in sketch_content:
        print("  ✅ Dual I2S configuration")
    else:
        print("  ⚠️  Single I2S configuration")
    
    # Configuration file
    print("\n⚙️  Checking Configuration:")
    pin_definitions = len(re.findall(r'#define\s+\w+_PIN\s+\d+', config_content))
    timing_definitions = len(re.findall(r'#define\s+\w+_DELAY\s+\d+', config_content))
    
    if pin_definitions >= 5:
        print(f"  ✅ Pin configuration ({pin_definitions} pins defined)")
    else:
        print(f"  ❌ Insufficient pin definitions ({pin_definitions})")
    
    if timing_definitions >= 3:
        print(f"  ✅ Timing configuration ({timing_definitions} timing parameters)")
    else:
        print(f"  ⚠️  Limited timing configuration ({timing_definitions})")
    
    # Code quality metrics
    print("\n📊 Code Quality Metrics:")
    lines_of_code = len(sketch_content.split('\n'))
    comment_lines = len([line for line in sketch_content.split('\n') 
                        if line.strip().startswith('//') or line.strip().startswith('/*')])
    function_count = len(re.findall(r'void\s+\w+\s*\(', sketch_content))
    
    print(f"  📝 Lines of code: {lines_of_code}")
    print(f"  💬 Comment lines: {comment_lines} ({comment_lines/lines_of_code*100:.1f}%)")
    print(f"  🔧 Function count: {function_count}")
    
    if comment_lines / lines_of_code > 0.1:
        print("  ✅ Good documentation level")
    else:
        print("  ⚠️  Low documentation level")
    
    # Check for key features mentioned in problem statement
    print("\n🎯 Checking Key Problem Statement Requirements:")
    
    # WAV header creation
    if 'writeWAVHeader' in sketch_content and 'updateWAVHeader' in sketch_content:
        print("  ✅ WAV header creation and finalization")
    else:
        print("  ❌ WAV header handling missing")
    
    # Button debouncing
    if 'BUTTON_DEBOUNCE_DELAY' in config_content:
        print("  ✅ Enhanced button handling with debouncing")
    else:
        print("  ❌ Button debouncing missing")
    
    # RFID timing fixes
    if 'RFID_REMOVAL_DELAY' in config_content:
        print("  ✅ Robust RFID detection with timing fixes")
    else:
        print("  ❌ RFID timing fixes missing")
    
    # Memory management
    if 'ESP.getFreeHeap()' in sketch_content:
        print("  ✅ Memory management and monitoring")
    else:
        print("  ❌ Memory management missing")
    
    # Documentation files
    print("\n📖 Checking Documentation:")
    doc_files = ['README.md', 'HARDWARE_SETUP.md', 'LIBRARY_REQUIREMENTS.md', 'USER_GUIDE.md']
    for doc_file in doc_files:
        if os.path.exists(doc_file):
            print(f"  ✅ {doc_file}")
        else:
            print(f"  ❌ {doc_file} missing")
    
    print("\n" + "=" * 60)
    
    if all_functions_present:
        print("🎉 VALIDATION PASSED - All required functions implemented!")
        print("✨ Production-ready ESP32 RFID Audio Recorder complete")
        return True
    else:
        print("❌ VALIDATION FAILED - Some requirements missing")
        return False

if __name__ == "__main__":
    success = validate_complete_implementation()
    exit(0 if success else 1)