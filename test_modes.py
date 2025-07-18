#!/usr/bin/env python3
import socket
import time
import threading

def create_client(name, password):
    """Create a client and authenticate"""
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.connect(('localhost', 6667))
    
    # Send commands
    sock.send(f"PASS {password}\r\n".encode())
    sock.send(f"NICK {name}\r\n".encode())
    sock.send(f"USER {name} 0 * :{name}\r\n".encode())
    
    return sock

def read_responses(sock, name):
    """Read responses from server"""
    try:
        while True:
            data = sock.recv(1024)
            if not data:
                break
            print(f"[{name}] {data.decode().strip()}")
    except:
        pass

def test_mode_commands():
    print("Testing IRC MODE Commands")
    print("========================")
    
    # Create admin client
    admin = create_client("admin", "mypassword")
    admin_thread = threading.Thread(target=read_responses, args=(admin, "admin"))
    admin_thread.daemon = True
    admin_thread.start()
    
    time.sleep(1)
    
    # Admin joins channel
    print("Admin joining #modetest...")
    admin.send(b"JOIN #modetest\r\n")
    time.sleep(1)
    
    # Test different MODE commands
    print("\n1. Testing +i (invite only)...")
    admin.send(b"MODE #modetest +i\r\n")
    time.sleep(1)
    
    print("\n2. Testing +t (topic restricted)...")
    admin.send(b"MODE #modetest +t\r\n")
    time.sleep(1)
    
    print("\n3. Testing +l (user limit)...")
    admin.send(b"MODE #modetest +l 3\r\n")
    time.sleep(1)
    
    print("\n4. Testing -i (remove invite only)...")
    admin.send(b"MODE #modetest -i\r\n")
    time.sleep(1)
    
    print("\n5. Testing invalid mode...")
    admin.send(b"MODE #modetest +x\r\n")
    time.sleep(1)
    
    print("\nMode test completed!")
    admin.close()

if __name__ == "__main__":
    test_mode_commands()
