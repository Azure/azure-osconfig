#!/bin/bash

function die() {
  echo "$1"
  exit 1
}

if [ $# -ne 1 ]; then
  die "Usage: $0 <diff-file>"
fi

diff_file="$1"
if [ ! -f "$diff_file" ]; then
  die "File $diff_file does not exist."
fi

while IFS= read -r line; do
    if [[ "$line" == diff* ]]; then
        FILE=$(echo "$line" | cut -d' ' -f4 | sed 's|^b/||')
    elif [[ "$line" =~ \+*TODO* ]]; then
        COMMENT=$(echo "$line" | sed 's/^+//' | xargs)
        ESCAPED_COMMENT=$(jq -Rn --arg str "$COMMENT" '$str')
        LINE_NUM=$(grep -n "$line" "$diff_file" | head -n1 | cut -d: -f1)
        echo "[$FILE:$LINE_NUM] $COMMENT"
        body="{ \
          \"body\": \"TEST COMMENT: \`TEST\`\", \
          \"commit_id\": \"$COMMIT_SHA\", \
          \"path\": \"$FILE\", \
          \"line\": $LINE_NUM, \
          \"subject_type\": \"line\", \
          \"side\": \"RIGHT\" \
        }"
        url="https://api.github.com/repos/$REPO/pulls/$PR_NUMBER/comments"
        echo "Posting comment to GitHub: $url"
        echo "Body:"
        echo "$body"
        curl -s -X POST \
            -H "Authorization: Bearer $GITHUB_TOKEN" \
            -H "Accept: application/vnd.github.v3+json" \
            "$url" \
            -d "$body"
    fi
done < "$diff_file"
