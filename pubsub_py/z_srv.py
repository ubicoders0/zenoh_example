#!/usr/bin/env python3
import zenoh
import time
import zenoh

handler = zenoh.handlers.DefaultHandler()

def on_query(q: zenoh.Query):
    """
    This handler is invoked when a client issues a query matching our key expression.
    You should reply (or reply_err) and then drop the query.
    """
    print(f"Received query: selector={q.selector}, payload={q.payload}")
    try:
        # Build a reply payload — here we just echo a message
        resp_payload = zenoh.ZBytes(b"Hello from srv!")
        # Reply on the same key expression
        q.reply(q.selector.key_expr, resp_payload)
    except Exception as e:
        # On error
        q.reply_err(zenoh.ZBytes(str(e).encode()))
    finally:
        q.drop()

def main():
    # open a session (with default config)
    with zenoh.open(zenoh.Config()) as session:
        # declare a queryable on a key expression “demo/hello”
        # use DefaultHandler to allow manual recv() style
        with session.declare_queryable("demo/hello", handler) as queryable:
            print("Queryable declared at key ‘demo/hello’ — waiting for queries...")
            # continuously receive queries
            while True:
                q = queryable.handler.recv()  # blocks until a query arrives
                on_query(q)
                # Sleep a bit to avoid busy loop
                time.sleep(0.1)

if __name__ == "__main__":
    main()
