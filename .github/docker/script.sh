#!/usr/bin/env bash

# -e: exit when any command fails
# -x: all executed commands are printed to the terminal
# -o pipefail: prevents errors in a pipeline from being masked
set -exo pipefail

SECTION_PREFIX=.${SCRIPT_NAME}.section
TEMP_FILE_1=.${SCRIPT_NAME}.parse
TEMP_FILE_2=.${SCRIPT_NAME}.concat

cd $WORKSPACE_DIR

# Mark generic commands
cat README.md | sed '/^ *```all.*$/,/^ *```$/ { s/^ *\$/_M_/; { :loop; /^_M_.*[^\\]\\$/ { n; s/^/_M_/; t loop } } }' > ${TEMP_FILE_1}
# Mark commands that depend on the distro
sed -i '/^ *```.*'"${DOCKER_IMAGE}"'.*/,/^ *```$/ { s/^ *\$/_M_/; { :loop; /^_M_.*[^\\]\\$/ { n; s/^/_M_/; t loop } } }' ${TEMP_FILE_1}
# Append a space to all lines that contain only the marker
sed -i '/^_M_$/ s/$/ /' ${TEMP_FILE_1}
# Comment out all lines that do not contain the marker
sed -i '/^_M_/! s/^/# /' ${TEMP_FILE_1}
# Remove the appended comment from all marked lines
sed -i '/^_M_/ s/<--.*//' ${TEMP_FILE_1}
# Remove the marker
sed -i 's/^_M_ //' ${TEMP_FILE_1}

# Debian
if [ "$DOCKER_IMAGE" = "ubuntu:22.04" ]; then
    apt update
    apt install -y gawk
fi

# Divide the file into predefined sections
awk -v section_prefix=$SECTION_PREFIX "/# <!--section:/,/# <!--section-end-->/"'{
    # Check if the line is the start of a section
    if ($0 ~ /# <!--section:/) {
        # Construct the output file name
        match($0, /# <!--section:(.*)-->/, array)
        output_file=sprintf("%s.%s", section_prefix, array[1])
    } else {
        # Skip lines starting with "# "
        if ($0 ~ /^# /) next
    }
    # Write the line to the output file
    print $0 > output_file
}' ${TEMP_FILE_1}

# Construct the test sequence according to the provided instructions
awk -v section_prefix=$SECTION_PREFIX -v output_file=$TEMP_FILE_2 "/# <!--tests:/,/# -->/"'{
    # Skip the starting line
    if (/^# <!--tests:/) {
        next
    }

    # Stop processing when the end marker is found
    if (/^# -->/) {
        exit
    }

    # Remove the leading "# " from $0
    sub(/^# /, "", $0)

    # Split the line at semicolons into an array
    n = split($0, array, ";")
    for (i = 1; i <= n; i++) {
        filename=sprintf("%s.%s", section_prefix, array[i])
        system("cat " filename " >> " output_file)
    }
}' ${TEMP_FILE_1}

# Initialize an executable script
cat > ${SCRIPT_NAME} << EOF
#!/usr/bin/env bash

set -exufo pipefail

# This is necessary when the shell is in non-interactive mode
shopt -s expand_aliases

alias sudo=""

EOF

cat ${TEMP_FILE_2} >> ${SCRIPT_NAME}
echo -e '\nexit 0' >> ${SCRIPT_NAME}

chmod a+x ${SCRIPT_NAME}
./${SCRIPT_NAME}

exit 0
