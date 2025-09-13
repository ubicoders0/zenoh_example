import time
import zenoh

def listener(sample):
    # Get raw bytes from the payload
    payload = sample.payload.to_bytes()
    print(f"Received on {sample.key_expr}: {payload}")

def main():
    session = zenoh.open(zenoh.Config())
    sub = session.declare_subscriber("vr/1/states", listener)

    print("Subscriber running. Press Ctrl+C to exit.")
    try:
        while True:
            time.sleep(1)
    except KeyboardInterrupt:
        pass
    finally:
        sub.undeclare()
        session.close()

if __name__ == "__main__":
    main()
