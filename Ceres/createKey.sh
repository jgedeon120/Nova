#!/bin/bash

BKSNAME=''
PEMNAME=''
STOREPASS=''

if [ $# -ne 3 ]; then
  echo 'Usage: createKey.sh BKS_NAME PEM_NAME STOREPASS'
  exit 1
else
  BKSNAME=$1
  PEMNAME=$2
  STOREPASS=$3
fi

CURRENT="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

if [ "$CURRENT" != "$HOME/Code/Nova/Ceres" ]; then
  echo 'Run this in the Ceres Directory'
  exit 1
fi

export CLASSPATH=libs/bcprov-jdk15on-146.jar
CERTSTORE=res/raw/$BKSNAME.bks
if [ -a $CERTSTORE ]; then
	rm $CERTSTORE || exit 1
fi

keytool -import -v -trustcacerts -alias 0 -file <(openssl x509 -in keys/$PEMNAME) -keystore $CERTSTORE -storetype BKS -provider org.bouncycastle.jce.provider.BouncyCastleProvider -providerpath /usr/share/java/bcprov.jar -storepass $STOREPASS

