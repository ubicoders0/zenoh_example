import time
import zenoh

def main():
    # Open Zenoh session
    session = zenoh.open(zenoh.Config())

    # Declare publisher for vr/0/cmd
    pub = session.declare_publisher("vr/1/cmd")

    print("Publisher running. Press Ctrl+C to stop.")
    try:
        i = 0
        while True:
            # Example dummy payload as bytes
            data = f"dummy-message-{i}".encode("utf-8")
            pub.put(data)
            print(f"Published: {data}")
            i += 1
            time.sleep(1)
    except KeyboardInterrupt:
        pass
    finally:
        pub.undeclare()
        session.close()

if __name__ == "__main__":
    main()
