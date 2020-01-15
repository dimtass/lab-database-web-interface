#!/bin/bash

: ${FOLDER_IN:="images"}
: ${FOLDER_OUT:="thumbs"}
: ${MAX_WIDTH:=320}
: ${MAX_HEIGHT:=320}

# Check if folders exists
function folder_exists()
{
    if [ ! -d "$1" ]; then
        echo "The folder $1 doesn't exist"
        exit 1
    fi
}

folder_exists ${FOLDER_IN}
folder_exists ${FOLDER_OUT}

echo "Resize images from ${FOLDER_IN} to ${FOLDER_OUT}"

find ${FOLDER_IN} -iname '*.jpg' -printf '%f\n' | xargs -n 1 -I {} convert ${FOLDER_IN}/{} -verbose -resize ${MAX_WIDTH}x${MAX_HEIGHT} ./${FOLDER_OUT}/{}
find ${FOLDER_IN} -iname '*.png' -printf '%f\n' | xargs -n 1 -I {} convert ${FOLDER_IN}/{} -verbose -resize ${MAX_WIDTH}x${MAX_HEIGHT} ./${FOLDER_OUT}/{}
