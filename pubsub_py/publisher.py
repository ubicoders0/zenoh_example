import time
import zenoh

def main():
    session = zenoh.open(zenoh.Config())
    pub = session.declare_publisher("vr/1/states")
    print("Publisher running. Press Ctrl+C to stop.")

    i = 0
    while True:
        data = f"dummy-message-{i}".encode("utf-8")
        pub.put(data)
        print(f"Published: {data}")
        i += 1
        time.sleep(0.01)

if __name__ == "__main__":
    main()
