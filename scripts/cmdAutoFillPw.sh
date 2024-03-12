#!/bin/bash

if [ "$#" -lt 2 ]; then
  echo "Usage: $0 password command [args...]"
  exit 1
fi

PASSWORD="$1"
shift
COMMAND="$*"

ret=$(/usr/bin/expect <<EOF
    set timeout -1
    spawn $COMMAND
    expect {
        "Enter Password or Pin*" {
            send -- "$PASSWORD\r"; exp_continue
        }
        eof
    }
    catch wait result
    exit [ lindex \$result 3 ]
EOF
)

rc=$?
echo "${ret[@]}"
exit $rc
