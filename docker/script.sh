#!/usr/bin/env bash
echo "running on host: $HOST"
echo "seeds: $SEEDS"
NAME=${NAME:-$HOST}
echo "name: ${NAME}"
echo /bin/node_keeper --name=$NAME --seeds=$SEEDS --host=$HOST --port=4445
/bin/node_keeper --name=$NAME --seeds=$SEEDS --host=$HOST --port=4445&
echo $APP --host=$HOST --name=$NAME $APP_ARGS
$APP --host=$HOST --name=$NAME $APP_ARGS
