#!/usr/bin/awk -f
# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

# Initialize variables to hold values
BEGIN {
    # Set the output format
    output = "";
}

# Process each line of input
{
    if ($1 == "ComponentName") {
        componentName = $3;
        gsub(/;/, "", componentName);  # Remove semicolon
        gsub(/"/, "", componentName);  # Remove quotes
    }
    if ($1 == "InitObjectName") {
        initObjectName = $3;
        gsub(/;/, "", initObjectName);  # Remove semicolon
        gsub(/"/, "", initObjectName);  # Remove quotes
    }
    if ($1 == "DesiredObjectName") {
        desiredObjectName = $3;
        gsub(/;/, "", desiredObjectName);  # Remove semicolon
        gsub(/"/, "", desiredObjectName);  # Remove quotes
    }
    if ($1 == "ReportedObjectName") {
        reportedObjectName = $3;
        gsub(/;/, "", reportedObjectName);  # Remove semicolon
        gsub(/"/, "", reportedObjectName);  # Remove quotes
    }
    if ($1 == "DesiredObjectValue") {
        payload = $0;
        gsub(/.*= /, "", payload); # Remove everything before the equals sign
        gsub(/;/, "", payload);  # Remove semicolon
        gsub(/"/, "", payload);  # Remove quotes
    }

    # Check for the end of an object
    if ($1 == "};") {
        if (componentName == "") {
            next;
        }
        key = componentName"."desiredObjectName;
        if (map[key] == 1) {
            # Reset variables for the next object
            componentName = "";
            initObjectName = "";
            desiredObjectName = "";
            reportedObjectName = "";
            payload = "";
            next;
        }
        # Append the output for the current object
        if (remediate == "1") {
            if (initObjectName != "") {
                output = output "  {\n";
                output = output "    \"ObjectType\": \"Desired\",\n";
                output = output "    \"ComponentName\": \"" componentName "\",\n";
                output = output "    \"ObjectName\": \"" initObjectName "\",\n";
                output = output "    \"Payload\": \"" payload "\"\n";
                output = output "  },\n";
            }
            output = output "  {\n";
            output = output "    \"ObjectType\": \"Desired\",\n";
            output = output "    \"ComponentName\": \"" componentName "\",\n";
            output = output "    \"ObjectName\": \"" desiredObjectName "\"";
            if (initObjectName != "") {
                output = output ",\n    \"Payload\": \"" payload "\"\n";
            } else {
                output = output "\n"
            }
            output = output "  },\n";
        } else {
            if (initObjectName != "") {
                output = output "  {\n";
                output = output "    \"ObjectType\": \"Desired\",\n";
                output = output "    \"ComponentName\": \"" componentName "\",\n";
                output = output "    \"ObjectName\": \"" initObjectName "\",\n";
                output = output "    \"Payload\": \"" payload "\"\n";
                output = output "  },\n";
            }
            output = output "  {\n";
            output = output "    \"ObjectType\": \"Reported\",\n";
            output = output "    \"ComponentName\": \"" componentName "\",\n";
            output = output "    \"ObjectName\": \"" reportedObjectName "\"\n";
            output = output "  },\n";
        }

        # Reset variables for the next object
        componentName = "";
        initObjectName = "";
        desiredObjectName = "";
        reportedObjectName = "";
        payload = "";
        map[key] = 1;
    }
}

# Print the final output
END {
    # Remove the last comma for valid JSON format
    sub(/,$/, "", output);
    printf output;
}
