#!/usr/bin/env bash
echo "running on host: $HOST"
echo "seeds: $SEEDS"
NAME=${NAME:-$HOST}
echo "name: ${NAME}"



if [ -n "$APP_ARGS" ] && [ -n "$APP" ]; then
    echo /bin/node_keeper --name=$NAME --seeds=$SEEDS --host=$HOST $PRINT_PING_LOG --port=4445 $LOG_COMMANDS --role=$ROLE --app=$APP --app-args="${APP_ARGS}"
    /bin/node_keeper --name=$NAME --seeds=$SEEDS --host=$HOST $PRINT_PING_LOG --port=4445 $LOG_COMMANDS --role=$ROLE --app=$APP --app-args="${APP_ARGS}"
    exit 0
fi

if [ -n "$APP" ]; then
  echo /bin/node_keeper --name=$NAME --seeds=$SEEDS --host=$HOST $PRINT_PING_LOG --port=4445 $LOG_COMMANDS --role=$ROLE --app=$APP
  /bin/node_keeper --name=$NAME --seeds=$SEEDS --host=$HOST $PRINT_PING_LOG --port=4445 $LOG_COMMANDS --role=$ROLE --app=$APP
  exit 0
fi

if [ -z "$APP_ARGS" ] && [ -z "$APP" ]; then
  echo /bin/node_keeper --name=$NAME --seeds=$SEEDS --host=$HOST $PRINT_PING_LOG --port=4445 $LOG_COMMANDS --role=$ROLE
  /bin/node_keeper --name=$NAME --seeds=$SEEDS --host=$HOST $PRINT_PING_LOG --port=4445 $LOG_COMMANDS --role=$ROLE
  exit 0
fi

echo /bin/yanghui_cluster_root_v2 ${APP_ARGS}
/bin/yanghui_cluster_root_v2 ${APP_ARGS}
