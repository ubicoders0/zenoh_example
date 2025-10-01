#!/usr/bin/env python3
import zenoh
import itertools
import zenoh

handler = zenoh.handlers.DefaultHandler()

def on_reply(reply: zenoh.Reply):
    if reply.ok:
        sample = reply.ok
        print(f"Got reply: {sample.key_expr} => {sample.payload.to_string()}")
    else:
        print(f"Error reply: {reply.err.payload.to_string()}")

def main():
    with zenoh.open(zenoh.Config()) as session:
        # declare a querier on the same key “demo/hello”
        querier = session.declare_querier("demo/hello", target=None, timeout=3.0)
        print("Querier ready — sending queries every second. Ctrl-C to stop.")
        for i in itertools.count():
            print(f"Sending query #{i}")
            querier.get(handler=on_reply)
            # wait before sending next
            import time; time.sleep(1)

if __name__ == "__main__":
    main()
