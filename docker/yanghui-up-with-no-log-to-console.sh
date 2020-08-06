#!/usr/bin/env bash
docker-compose -f yanghui_app.yml --env-file no_log_to_console.env up -d