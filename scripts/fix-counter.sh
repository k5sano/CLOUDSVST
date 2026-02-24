#!/bin/bash

COUNTER\_FILE=".copilot/handoff/.fix-count" MAX\_FIXES=3

case "$1" in increment) if \[ -f "$COUNTER\_FILE" \]; then count=(cat"COUNTER\_FILE") echo ((count+1))\>"COUNTER\_FILE" else echo 1 > "$COUNTER\_FILE" fi echo "Fix count: (cat"COUNTER\_FILE")/$MAX\_FIXES" ;; check) if \[ -f "$COUNTER\_FILE" \]; then count=(cat"COUNTER\_FILE") if \[ "count"−ge"MAX\_FIXES" \]; then echo "ERROR: Fix limit reached (count/MAX\_FIXES). Rolling back." echo "Please review the handoff files and try a different approach." exit 1 fi fi echo "Fix count OK: (cat"COUNTER\_FILE" 2>/dev/null || echo 0)/$MAX\_FIXES" ;; reset) echo 0 > "$COUNTER\_FILE" echo "Fix counter reset." ;; \*) echo "Usage: fix-counter.sh \[increment|check|reset\]" ;; esac