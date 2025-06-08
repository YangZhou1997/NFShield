#!/bin/bash

# Directory to search for large files
DATA_DIR="data"

# Directory to move original large files
ORIG_DIR="data_orig"

# Size threshold (100MB)
SIZE_THRESHOLD="100M"

# Size of each split file (50MB by default - you can modify this)
SPLIT_SIZE="10M"

# Check if Data directory exists
if [ ! -d "$DATA_DIR" ]; then
    echo "Error: $DATA_DIR directory does not exist"
    exit 1
fi

# Create DataOrig directory if it doesn't exist
if [ ! -d "$ORIG_DIR" ]; then
    echo "Creating $ORIG_DIR directory for original files..."
    mkdir -p "$ORIG_DIR"
fi

echo "Searching for files larger than $SIZE_THRESHOLD in $DATA_DIR..."
echo "Each file will be split into chunks of $SPLIT_SIZE"
echo "---"

# Find all files larger than 100MB in Data directory recursively
find "$DATA_DIR" -type f -size +$SIZE_THRESHOLD | while read -r file; do
    echo "Processing: $file"
    
    # Get file size for information
    file_size=$(ls -lh "$file" | awk '{print $5}')
    echo "File size: $file_size"
    
    # Get the directory and filename
    dir=$(dirname "$file")
    filename=$(basename "$file")
    
    # Create split files with the naming pattern: filename.part.000, filename.part.001, etc.
    # Using -d for numeric suffixes and -a 3 for 3-digit suffixes (000, 001, 002, etc.)
    split -b "$SPLIT_SIZE" -d -a 3 "$file" "$dir/$filename.part."
    
    if [ $? -eq 0 ]; then
        echo "✓ Successfully split $file into chunks:"
        
        # Generate MD5 hash of original file before moving it
        echo "  Generating MD5 hash of original file..."
        if command -v md5 >/dev/null 2>&1; then
            original_md5=$(md5 -q "$file")
        elif command -v md5sum >/dev/null 2>&1; then
            original_md5=$(md5sum "$file" | cut -d' ' -f1)
        else
            echo "  ! MD5 command not available, skipping hash generation"
            original_md5=""
        fi
        
        if [ -n "$original_md5" ]; then
            echo "  Original file MD5: $original_md5"
            # Save the MD5 hash to a file for later verification
            echo "$original_md5  $filename" > "$dir/$filename.md5"
            echo "  MD5 hash saved to: $dir/$filename.md5"
        fi
        
        # List the created parts
        ls -la "$dir/$filename.part."* | while read -r line; do
            echo "  $line"
        done
        
        # Count the number of parts created
        part_count=$(ls "$dir/$filename.part."* 2>/dev/null | wc -l)
        echo "  Total parts created: $part_count"
        
        # Create the corresponding directory structure in DataOrig
        relative_path="${file#$DATA_DIR/}"
        orig_file_path="$ORIG_DIR/$relative_path"
        orig_dir_path=$(dirname "$orig_file_path")
        
        # Create directory structure in DataOrig if needed
        mkdir -p "$orig_dir_path"
        
        # Move the original file to DataOrig
        mv "$file" "$orig_file_path"
        if [ $? -eq 0 ]; then
            echo "  ✓ Original file moved to: $orig_file_path"
        else
            echo "  ✗ Error moving original file to DataOrig"
        fi
    else
        echo "✗ Error splitting $file"
    fi
    echo "---"
done

echo "File splitting operation complete."
echo ""
echo "Note: Original large files have been moved to the $ORIG_DIR directory"
echo "to preserve the original files while keeping the Data directory clean."
