import time
import zenoh

def listener(sample):
    payload = sample.payload.to_bytes()
    print(f"Received on {sample.key_expr}: {payload}")

def main():
    session = zenoh.open(zenoh.Config())
    sub = session.declare_subscriber("vr/1/states", listener)
    print("Subscriber running. Press Ctrl+C to exit.")
    while True:
        time.sleep(1)

if __name__ == "__main__":
    main()
