#!/bin/sh

#if [ ! dpkg -s netcat ]; then
apt-get update && apt-get install netcat -y
#fi

wait_for() {
  HOST=$1
  PORT=$2
  
  sleep 2
  echo "[+] waiting for port ${PORT} on ${HOST} to accept connections ..."
  while ! nc -z $HOST $PORT;
  do
    sleep 2;
  done
}

is_command_arg=false
command=""
for arg in "$@"
do
  echo $arg
  if [ "${arg}" = "--" ]; then
    is_command_arg=true
    continue
  fi

  if [ "${is_command_arg}" = true ]; then
    command="${command} ${arg}"
  else
    HOST=${arg%:*}
    PORT=${arg#*:}
    wait_for $HOST $PORT
  fi;
done

${command}
