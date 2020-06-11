#!/usr/bin/env bash
echo "running on host: $HOST"
echo "seeds: $SEEDS"
NAME=${NAME:-$HOST}
echo "name: ${NAME}"
if [ -n "$APP_ARGS" ]; then
  OPTION_APP_ARGS="--app-args='${APP_ARGS}'"
fi
echo /bin/node_keeper --name=$NAME --seeds=$SEEDS --host=$HOST --port=4445 $LOG_COMMANDS --app=$APP OPTION_APP_ARGS
/bin/node_keeper --name=$NAME --seeds=$SEEDS --host=$HOST --port=4445 $LOG_COMMANDS --app=$APP OPTION_APP_ARGS
