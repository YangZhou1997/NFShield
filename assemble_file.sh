#!/bin/bash

# Directory to search for split files
DATA_DIR="data"

# Directory to move split files after reassembly
ORIG_DIR="data_orig"

# Check if Data directory exists
if [ ! -d "$DATA_DIR" ]; then
    echo "Error: $DATA_DIR directory does not exist"
    exit 1
fi

# Create DataOrig directory if it doesn't exist
if [ ! -d "$ORIG_DIR" ]; then
    echo "Creating $ORIG_DIR directory for split files..."
    mkdir -p "$ORIG_DIR"
fi

echo "Searching for split files (*.part.*) in $DATA_DIR..."
echo "Files will be reassembled and split files moved to $ORIG_DIR"
echo ""
echo "Debug: Found these part files:"
find "$DATA_DIR" -name "*.part.*" -type f | head -5
echo ""
echo "Debug: Extracted base files:"
find "$DATA_DIR" -name "*.part.*" -type f | sed 's/\.part\.[0-9][0-9]*$//' | sort -u | head -3
echo "---"

# Find all .part.* files and extract unique base filenames
find "$DATA_DIR" -name "*.part.*" -type f | sed 's/\.part\.[0-9][0-9]*$//' | sort -u | while read -r base_file; do
    if [ -z "$base_file" ]; then
        continue
    fi
    
    echo "Processing base file: $base_file"
    
    # Find all parts for this base file and sort them numerically
    # Use direct pattern matching with the full base file path
    part_files=($(ls "$base_file".part.* 2>/dev/null | sort -V))
    
    if [ ${#part_files[@]} -eq 0 ]; then
        echo "  ✗ No part files found for $base_file"
        continue
    fi
    
    echo "  Found ${#part_files[@]} part files:"
    for part in "${part_files[@]}"; do
        echo "    $(basename "$part")"
    done
    
    # Check if target file already exists
    if [ -f "$base_file" ]; then
        echo "  Warning: Target file $base_file already exists. Skipping reassembly."
        echo "  (Delete the existing file if you want to reassemble)"
        continue
    fi
    
    # Reassemble the file by concatenating all parts in order
    echo "  Reassembling into: $base_file"
    
    # Create directory for output file if needed
    output_dir=$(dirname "$base_file")
    mkdir -p "$output_dir"
    
    # Concatenate all parts
    cat "${part_files[@]}" > "$base_file"
    
    if [ $? -eq 0 ]; then
        echo "  ✓ Successfully reassembled $base_file"
        
        # Get size of reassembled file
        file_size=$(ls -lh "$base_file" | awk '{print $5}')
        echo "  Reassembled file size: $file_size"
        
        # Verify integrity using saved MD5 hash or by comparing with original file in DataOrig
        relative_path="${base_file#$DATA_DIR/}"
        original_file="$ORIG_DIR/$relative_path"
        md5_file="$base_file.md5"
        
        # First try to verify using saved MD5 hash
        if [ -f "$md5_file" ]; then
            echo "  Verifying integrity using saved MD5 hash..."
            saved_md5=$(cut -d' ' -f1 "$md5_file")
            
            if command -v md5 >/dev/null 2>&1; then
                new_md5=$(md5 -q "$base_file")
            elif command -v md5sum >/dev/null 2>&1; then
                new_md5=$(md5sum "$base_file" | cut -d' ' -f1)
            else
                echo "  ! MD5 command not available"
                new_md5=""
            fi
            
            if [ -n "$new_md5" ] && [ -n "$saved_md5" ]; then
                if [ "$saved_md5" = "$new_md5" ]; then
                    echo "  ✓ MD5 verification passed - reassembled file matches original!"
                    echo "    MD5: $new_md5"
                else
                    echo "  ✗ MD5 verification failed - files differ!"
                    echo "    Expected: $saved_md5"
                    echo "    Got: $new_md5"
                fi
            fi
        elif [ -f "$original_file" ]; then
            echo "  Verifying integrity against original file..."
            
            # Compare file sizes
            orig_size=$(stat -f%z "$original_file" 2>/dev/null || stat -c%s "$original_file" 2>/dev/null)
            new_size=$(stat -f%z "$base_file" 2>/dev/null || stat -c%s "$base_file" 2>/dev/null)
            
            if [ "$orig_size" = "$new_size" ]; then
                echo "  ✓ File sizes match ($orig_size bytes)"
                
                # Compare checksums for complete verification
                if command -v md5 >/dev/null 2>&1; then
                    orig_md5=$(md5 -q "$original_file")
                    new_md5=$(md5 -q "$base_file")
                elif command -v md5sum >/dev/null 2>&1; then
                    orig_md5=$(md5sum "$original_file" | cut -d' ' -f1)
                    new_md5=$(md5sum "$base_file" | cut -d' ' -f1)
                else
                    echo "  ! MD5 command not available, skipping checksum verification"
                    orig_md5=""
                    new_md5=""
                fi
                
                if [ -n "$orig_md5" ] && [ -n "$new_md5" ]; then
                    if [ "$orig_md5" = "$new_md5" ]; then
                        echo "  ✓ Checksums match - files are identical!"
                    else
                        echo "  ✗ Checksums differ - reassembly may have corrupted data!"
                        echo "    Original: $orig_md5"
                        echo "    Reassembled: $new_md5"
                    fi
                fi
            else
                echo "  ✗ File sizes differ!"
                echo "    Original: $orig_size bytes"
                echo "    Reassembled: $new_size bytes"
            fi
        else
            echo "  ! Original file not found in $ORIG_DIR for comparison"
        fi
        
        # Move part files to DataOrig, preserving directory structure
        echo "  Moving part files to $ORIG_DIR..."
        moved_count=0
        failed_count=0
        
        for part_file in "${part_files[@]}"; do
            # Calculate relative path using bash string manipulation (portable across systems)
            relative_path="${part_file#$DATA_DIR/}"
            orig_part_path="$ORIG_DIR/$relative_path"
            orig_part_dir=$(dirname "$orig_part_path")
            
            # Create directory structure in DataOrig if needed
            mkdir -p "$orig_part_dir"
            
            # Move the part file
            mv "$part_file" "$orig_part_path"
            if [ $? -eq 0 ]; then
                ((moved_count++))
            else
                echo "    ✗ Failed to move $(basename "$part_file")"
                ((failed_count++))
            fi
        done
        
        echo "  ✓ Moved $moved_count part files to $ORIG_DIR"
        if [ $failed_count -gt 0 ]; then
            echo "  ✗ Failed to move $failed_count part files"
        fi
        
    else
        echo "  ✗ Error reassembling $base_file"
    fi
    echo "---"
done

echo "File reassembly operation complete."
echo ""
echo "Note: Reassembled files are now in their original locations in $DATA_DIR"
echo "Split files have been moved to $ORIG_DIR to keep things organized."
