#!/bin/bash
set -e

CURDIR=$(cd $(dirname "$0"); pwd)
# Get BUILDDIR and REAL_BITCREDITD
. "${CURDIR}/tests-config.sh"

export BITCREDITCLI=${BUILDDIR}/qa/pull-tester/run-bitcredit-cli
export BITCREDITD=${REAL_BITCREDITD}

if [ "x${EXEEXT}" = "x.exe" ]; then
  echo "Win tests currently disabled"
  exit 0
fi

#Run the tests

testScripts=(
    'wallet.py'
    'listtransactions.py'
    'mempool_resurrect_test.py'
    'txn_doublespend.py'
    'txn_doublespend.py --mineblock'
    'getchaintips.py'
    'rest.py'
    'mempool_spendcoinbase.py'
    'mempool_coinbase_spends.py'
    'httpbasics.py'
#    'forknotify.py'
);
if [ "x${ENABLE_BITCREDITD}${ENABLE_UTILS}${ENABLE_WALLET}" = "x111" ]; then
    for (( i = 0; i < ${#testScripts[@]}; i++ ))
    do
        if [ -z "$1" ] || [ "$1" == "${testScripts[$i]}" ] || [ "$1.py" == "${testScripts[$i]}" ]
        then
            echo -e "running testscript \033[1m${testScripts[$i]}...\033[0m"
            ${BUILDDIR}/qa/rpc-tests/${testScripts[$i]} --srcdir "${BUILDDIR}/src"
        fi
    done
else
  echo "No rpc tests to run. Wallet, utils, and bitcreditd must all be enabled"
fi
