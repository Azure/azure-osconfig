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

LINE_NUM=0
while IFS= read -r line; do
    LINE_NUM=$((LINE_NUM + 1))
    if [[ "$line" == diff* ]]; then
        FILE=$(echo "$line" | cut -d' ' -f4 | sed 's|^b/||')
        echo "FILE: $FILE"
    elif [[ "$line" =~ @@*@@* ]]; then
        LINE_NUM=0
        echo "Hunk: $line for file $FILE"
    elif [[ "$line" =~ \+*TODO* ]]; then
        COMMENT=$(echo "$line" | sed 's/^+//' | xargs)
        ESCAPED_COMMENT=$(jq -r -n --arg str "$COMMENT" '$str')
        echo "[$FILE:$LINE_NUM] $COMMENT"
        body="{ \
          \"body\": \"TEST COMMENT: \`$ESCAPED_COMMENT\`\", \
          \"commit_id\": \"$COMMIT_SHA\", \
          \"path\": \"$FILE\", \
          \"position\": $LINE_NUM, \
          \"side\": \"RIGHT\" \
        }"
        # mkdir -p "/tmp/$FILE"
        # echo $body > "/tmp/$FILE/comment.json"
        url="https://api.github.com/repos/$REPO/pulls/$PR_NUMBER/comments"
        # echo "Posting comment to GitHub: $url"
        # echo "Body:"
        # echo "$body"
        curl -s -X POST \
            -H "Authorization: Bearer $GITHUB_TOKEN" \
            -H "Accept: application/vnd.github.v3+json" \
            "$url" \
            -d "$body"
    fi
done < "$diff_file"
