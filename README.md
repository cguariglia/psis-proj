# TODO
- Thread cancellation: cancellation handlers! check where they're needed and get them done
- Server main.c
- Finish local client api functions
- Remote client communication/functions
- Decide on the communication/synchronisation system between remote clipboards
- Take care of signals (block on every thread except the main one, that handles them)

# Tweaks
- Server shutdown routine
  - Disconnect from all clients and remote clipboards
- Optimize rwlock to a fair queue-like system
- Meme cleansing
- ...
