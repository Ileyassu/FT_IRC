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
            print(f"[{name}] Received: {data.decode().strip()}")
    except:
        pass

def test_operator_commands():
    print("Testing IRC Server Operator Commands")
    print("====================================")
    
    # Create admin client (first to join becomes operator)
    admin = create_client("admin", "mypassword")
    
    # Start reading responses for admin
    admin_thread = threading.Thread(target=read_responses, args=(admin, "admin"))
    admin_thread.daemon = True
    admin_thread.start()
    
    time.sleep(1)
    
    # Admin joins a channel
    print("\n1. Admin joining #test channel...")
    admin.send(b"JOIN #test\r\n")
    time.sleep(1)
    
    # Create regular user
    user = create_client("user1", "mypassword")
    user_thread = threading.Thread(target=read_responses, args=(user, "user1"))
    user_thread.daemon = True
    user_thread.start()
    
    time.sleep(1)
    
    # User joins the same channel
    print("\n2. User1 joining #test channel...")
    user.send(b"JOIN #test\r\n")
    time.sleep(1)
    
    # Test TOPIC command (operator only)
    print("\n3. Testing TOPIC command (admin)...")
    admin.send(b"TOPIC #test :This is the new topic\r\n")
    time.sleep(1)
    
    # Test user trying TOPIC (should fail)
    print("\n4. Testing TOPIC command (user1 - should fail)...")
    user.send(b"TOPIC #test :User trying to change topic\r\n")
    time.sleep(1)
    
    # Test MODE command - set channel limit
    print("\n5. Testing MODE command - set limit (admin)...")
    admin.send(b"MODE #test +l 5\r\n")
    time.sleep(1)
    
    # Test MODE command - user trying (should fail)
    print("\n6. Testing MODE command (user1 - should fail)...")
    user.send(b"MODE #test +l 10\r\n")
    time.sleep(1)
    
    # Create another user for KICK test
    user2 = create_client("user2", "mypassword")
    user2_thread = threading.Thread(target=read_responses, args=(user2, "user2"))
    user2_thread.daemon = True
    user2_thread.start()
    
    time.sleep(1)
    
    # User2 joins channel
    print("\n7. User2 joining #test channel...")
    user2.send(b"JOIN #test\r\n")
    time.sleep(1)
    
    # Test KICK command
    print("\n8. Testing KICK command (admin kicks user2)...")
    admin.send(b"KICK #test user2 :You have been kicked\r\n")
    time.sleep(1)
    
    # Test INVITE command
    print("\n9. Testing INVITE command (admin invites user2)...")
    admin.send(b"INVITE user2 #test\r\n")
    time.sleep(1)
    
    # Test user trying KICK (should fail)
    print("\n10. Testing KICK command (user1 - should fail)...")
    user.send(b"KICK #test admin :Trying to kick admin\r\n")
    time.sleep(2)
    
    print("\nTest completed!")
    
    # Close connections
    admin.close()
    user.close()
    user2.close()

if __name__ == "__main__":
    test_operator_commands()
