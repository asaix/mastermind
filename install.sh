#!/bin/bash
# installs the mastermind binary next to this script to ~/.local/bin
set -e

cd "$(dirname "$0")"
mkdir -p "$HOME/.local/bin"
install -m 755 mastermind "$HOME/.local/bin/mastermind"
echo "installed mastermind to $HOME/.local/bin/mastermind"
