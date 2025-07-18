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

def test_multi_channel():
    print("Testing Multi-Channel Operator Privileges")
    print("========================================")
    
    # Create two users
    user1 = create_client("user1", "mypassword")
    user2 = create_client("user2", "mypassword")
    
    user1_thread = threading.Thread(target=read_responses, args=(user1, "user1"))
    user2_thread = threading.Thread(target=read_responses, args=(user2, "user2"))
    user1_thread.daemon = True
    user2_thread.daemon = True
    user1_thread.start()
    user2_thread.start()
    
    time.sleep(1)
    
    # User1 creates #channel1
    print("User1 creating #channel1...")
    user1.send(b"JOIN #channel1\r\n")
    time.sleep(1)
    
    # User2 creates #channel2
    print("User2 creating #channel2...")
    user2.send(b"JOIN #channel2\r\n")
    time.sleep(1)
    
    # User1 joins #channel2 (should not be operator)
    print("User1 joining #channel2...")
    user1.send(b"JOIN #channel2\r\n")
    time.sleep(1)
    
    # User2 joins #channel1 (should not be operator)
    print("User2 joining #channel1...")
    user2.send(b"JOIN #channel1\r\n")
    time.sleep(1)
    
    # Test operator privileges
    print("\nTesting operator privileges:")
    print("User1 trying to set topic in #channel1 (should work)...")
    user1.send(b"TOPIC #channel1 :User1's channel\r\n")
    time.sleep(1)
    
    print("User1 trying to set topic in #channel2 (should fail)...")
    user1.send(b"TOPIC #channel2 :User1 trying to take over\r\n")
    time.sleep(1)
    
    print("User2 trying to set topic in #channel2 (should work)...")
    user2.send(b"TOPIC #channel2 :User2's channel\r\n")
    time.sleep(1)
    
    print("User2 trying to set topic in #channel1 (should fail)...")
    user2.send(b"TOPIC #channel1 :User2 trying to take over\r\n")
    time.sleep(1)
    
    print("\nMulti-channel test completed!")
    user1.close()
    user2.close()

if __name__ == "__main__":
    test_multi_channel()
