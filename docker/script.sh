#!/usr/bin/env bash
echo "running on host: $HOST"
echo "seeds: $SEEDS"
NAME=${NAME:-$HOST}
echo "name: ${NAME}"
if [ -n "$APP_ARGS" ]; then
  echo /bin/node_keeper --name=$NAME --seeds=$SEEDS --host=$HOST --port=4445 $LOG_COMMANDS --app=$APP --app-args="${APP_ARGS}"
  /bin/node_keeper --name=$NAME --seeds=$SEEDS --host=$HOST --port=4445 $LOG_COMMANDS --app=$APP --app-args="${APP_ARGS}"
  exit 0
fi
echo /bin/node_keeper --name=$NAME --seeds=$SEEDS --host=$HOST --port=4445 $LOG_COMMANDS --app=$APP
/bin/node_keeper --name=$NAME --seeds=$SEEDS --host=$HOST --port=4445 $LOG_COMMANDS --app=$APP
