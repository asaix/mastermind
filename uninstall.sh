#!/bin/bash
# removes the mastermind binary installed by install.sh
set -e

rm -f "$HOME/.local/bin/mastermind"
echo "removed $HOME/.local/bin/mastermind"
