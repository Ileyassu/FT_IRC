#!/usr/bin/env python3
"""
ft_irc Performance Testing Script
Simulates thousands of concurrent users to test server performance
Tests memory usage, connection limits, message throughput, and stability
"""

import socket
import threading
import time
import random
import argparse
import sys
import queue
import statistics
from typing import List, Dict, Optional
from concurrent.futures import ThreadPoolExecutor, as_completed
import psutil
import os

class PerformanceStats:
    def __init__(self):
        self.successful_connections = 0
        self.failed_connections = 0
        self.successful_joins = 0
        self.failed_joins = 0
        self.messages_sent = 0
        self.messages_received = 0
        self.connection_times = []
        self.message_response_times = []
        self.errors = []
        self.start_time = time.time()
        self.lock = threading.Lock()
    
    def add_connection_success(self, connection_time):
        with self.lock:
            self.successful_connections += 1
            self.connection_times.append(connection_time)
    
    def add_connection_failure(self):
        with self.lock:
            self.failed_connections += 1
    
    def add_join_success(self):
        with self.lock:
            self.successful_joins += 1
    
    def add_join_failure(self):
        with self.lock:
            self.failed_joins += 1
    
    def add_message_sent(self):
        with self.lock:
            self.messages_sent += 1
    
    def add_message_received(self, response_time=None):
        with self.lock:
            self.messages_received += 1
            if response_time:
                self.message_response_times.append(response_time)
    
    def add_error(self, error):
        with self.lock:
            self.errors.append(error)
    
    def get_summary(self):
        with self.lock:
            total_time = time.time() - self.start_time
            avg_conn_time = statistics.mean(self.connection_times) if self.connection_times else 0
            avg_response_time = statistics.mean(self.message_response_times) if self.message_response_times else 0
            
            return {
                'total_time': total_time,
                'successful_connections': self.successful_connections,
                'failed_connections': self.failed_connections,
                'connection_success_rate': (self.successful_connections / (self.successful_connections + self.failed_connections) * 100) if (self.successful_connections + self.failed_connections) > 0 else 0,
                'successful_joins': self.successful_joins,
                'failed_joins': self.failed_joins,
                'messages_sent': self.messages_sent,
                'messages_received': self.messages_received,
                'avg_connection_time': avg_conn_time,
                'avg_response_time': avg_response_time,
                'errors_count': len(self.errors),
                'connections_per_second': self.successful_connections / total_time if total_time > 0 else 0
            }

class LoadTestClient:
    def __init__(self, client_id: int, host: str, port: int, password: str, stats: PerformanceStats):
        self.client_id = client_id
        self.host = host
        self.port = port
        self.password = password
        self.stats = stats
        self.nickname = f"TestUser{client_id:05d}"
        self.socket = None
        self.connected = False
        self.authenticated = False
        self.channels_joined = set()
        self.messages_received = []
        self.running = False
        self.listener_thread = None
        
    def connect(self, timeout=30) -> bool:
        """Connect to IRC server with timeout"""
        start_time = time.time()
        try:
            self.socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            self.socket.settimeout(timeout)
            self.socket.connect((self.host, self.port))
            self.connected = True
            self.running = True
            
            # Start message listener
            self.listener_thread = threading.Thread(target=self._listen, daemon=True)
            self.listener_thread.start()
            
            # Send authentication
            if self.password:
                self._send(f"PASS {self.password}")
            self._send(f"NICK {self.nickname}")
            self._send(f"USER {self.nickname} 0 * :{self.nickname}")
            
            # Wait for authentication
            auth_timeout = 10
            auth_start = time.time()
            while time.time() - auth_start < auth_timeout:
                if any('001' in msg for msg in self.messages_received[-5:]):
                    self.authenticated = True
                    connection_time = time.time() - start_time
                    self.stats.add_connection_success(connection_time)
                    return True
                time.sleep(0.1)
            
            self.stats.add_connection_failure()
            self.stats.add_error(f"Client {self.client_id}: Authentication timeout")
            return False
            
        except Exception as e:
            self.stats.add_connection_failure()
            self.stats.add_error(f"Client {self.client_id}: Connection failed - {e}")
            return False
    
    def _send(self, message: str):
        """Send message to server"""
        if self.connected and self.socket:
            try:
                full_message = f"{message}\r\n"
                self.socket.send(full_message.encode('utf-8'))
                return True
            except Exception as e:
                self.stats.add_error(f"Client {self.client_id}: Send error - {e}")
                return False
        return False
    
    def _listen(self):
        """Listen for messages from server"""
        buffer = ""
        while self.running and self.connected:
            try:
                self.socket.settimeout(1)
                data = self.socket.recv(1024).decode('utf-8')
                if not data:
                    break
                
                buffer += data
                while '\r\n' in buffer:
                    line, buffer = buffer.split('\r\n', 1)
                    if line:
                        self.messages_received.append(line)
                        self.stats.add_message_received()
                        
                        # Handle PING automatically
                        if line.startswith('PING'):
                            pong = line.replace('PING', 'PONG')
                            self._send(pong)
                        
            except socket.timeout:
                continue
            except Exception as e:
                if self.running:
                    self.stats.add_error(f"Client {self.client_id}: Listen error - {e}")
                break
    
    def join_channel(self, channel: str) -> bool:
        """Join a channel"""
        if self._send(f"JOIN {channel}"):
            self.stats.add_message_sent()
            
            # Wait for join confirmation
            start_time = time.time()
            while time.time() - start_time < 5:
                recent_messages = self.messages_received[-10:]
                if any(f"JOIN {channel}" in msg for msg in recent_messages):
                    self.channels_joined.add(channel)
                    self.stats.add_join_success()
                    return True
                time.sleep(0.1)
            
            self.stats.add_join_failure()
            return False
        return False
    
    def send_channel_message(self, channel: str, message: str) -> bool:
        """Send message to channel"""
        if self._send(f"PRIVMSG {channel} :{message}"):
            self.stats.add_message_sent()
            return True
        return False
    
    def send_private_message(self, target: str, message: str) -> bool:
        """Send private message"""
        if self._send(f"PRIVMSG {target} :{message}"):
            self.stats.add_message_sent()
            return True
        return False
    
    def disconnect(self):
        """Disconnect from server"""
        self.running = False
        if self.connected:
            try:
                self._send("QUIT :Load test completed")
                time.sleep(0.1)
                self.socket.close()
            except:
                pass
            self.connected = False

class IRCPerformanceTester:
    def __init__(self, host: str, port: int, password: str = None):
        self.host = host
        self.port = port
        self.password = password
        self.stats = PerformanceStats()
        self.clients: List[LoadTestClient] = []
        self.test_channels = ["#general", "#test", "#performance", "#load", "#stress"]
        
    def test_connection_capacity(self, max_users: int, batch_size: int = 50, delay_between_batches: float = 0.1):
        """Test how many concurrent connections the server can handle"""
        print(f"🔗 Testing connection capacity with {max_users} users...")
        print(f"📦 Batch size: {batch_size}, Delay between batches: {delay_between_batches}s")
        
        def connect_client(client_id):
            client = LoadTestClient(client_id, self.host, self.port, self.password, self.stats)
            success = client.connect()
            if success:
                self.clients.append(client)
                return client
            return None
        
        # Connect clients in batches
        with ThreadPoolExecutor(max_workers=batch_size) as executor:
            for batch_start in range(0, max_users, batch_size):
                batch_end = min(batch_start + batch_size, max_users)
                
                print(f"📊 Connecting batch {batch_start+1}-{batch_end} of {max_users}...")
                
                # Submit batch
                futures = []
                for i in range(batch_start, batch_end):
                    future = executor.submit(connect_client, i)
                    futures.append(future)
                
                # Wait for batch completion
                for future in as_completed(futures):
                    try:
                        result = future.result(timeout=30)
                    except Exception as e:
                        self.stats.add_error(f"Batch connection error: {e}")
                
                # Brief pause between batches
                if batch_end < max_users:
                    time.sleep(delay_between_batches)
                
                # Print progress
                current_stats = self.stats.get_summary()
                print(f"✅ Connected: {current_stats['successful_connections']}, "
                      f"❌ Failed: {current_stats['failed_connections']}, "
                      f"📈 Rate: {current_stats['connections_per_second']:.1f}/sec")
        
        print(f"🎯 Connection test completed. {len(self.clients)} clients connected successfully.")
    
    def test_channel_operations(self):
        """Test all clients joining channels"""
        print(f"🏠 Testing channel operations with {len(self.clients)} clients...")
        
        def join_channels(client):
            try:
                # Each client joins 1-3 random channels
                channels_to_join = random.sample(self.test_channels, random.randint(1, 3))
                for channel in channels_to_join:
                    client.join_channel(channel)
                    time.sleep(random.uniform(0.1, 0.5))  # Stagger joins
                return True
            except Exception as e:
                self.stats.add_error(f"Channel join error for client {client.client_id}: {e}")
                return False
        
        # Join channels in parallel
        with ThreadPoolExecutor(max_workers=100) as executor:
            futures = [executor.submit(join_channels, client) for client in self.clients]
            
            completed = 0
            for future in as_completed(futures):
                completed += 1
                if completed % 100 == 0:
                    print(f"📊 Channel joins completed: {completed}/{len(self.clients)}")
        
        current_stats = self.stats.get_summary()
        print(f"🎯 Channel operations completed. Successful joins: {current_stats['successful_joins']}")
    
    def test_message_throughput(self, duration: int = 60, messages_per_client: int = 10):
        """Test message throughput"""
        print(f"💬 Testing message throughput for {duration}s with {len(self.clients)} clients...")
        print(f"📨 Each client will send ~{messages_per_client} messages")
        
        def send_messages(client):
            try:
                messages_sent = 0
                start_time = time.time()
                
                while time.time() - start_time < duration and messages_sent < messages_per_client:
                    # Choose random channel from joined channels
                    if client.channels_joined:
                        channel = random.choice(list(client.channels_joined))
                        message = f"Test message {messages_sent} from {client.nickname}"
                        client.send_channel_message(channel, message)
                        messages_sent += 1
                        
                        # Random delay between messages
                        time.sleep(random.uniform(1, 5))
                    else:
                        time.sleep(1)
                
                return messages_sent
            except Exception as e:
                self.stats.add_error(f"Message sending error for client {client.client_id}: {e}")
                return 0
        
        # Send messages in parallel
        start_time = time.time()
        with ThreadPoolExecutor(max_workers=50) as executor:
            futures = [executor.submit(send_messages, client) for client in self.clients[:1000]]  # Limit to 1000 for throughput test
            
            # Monitor progress
            while any(not f.done() for f in futures):
                current_stats = self.stats.get_summary()
                elapsed = time.time() - start_time
                print(f"📊 Messages - Sent: {current_stats['messages_sent']}, "
                      f"Received: {current_stats['messages_received']}, "
                      f"Rate: {current_stats['messages_sent']/elapsed:.1f}/sec")
                time.sleep(5)
        
        current_stats = self.stats.get_summary()
        print(f"🎯 Message throughput test completed. "
              f"Sent: {current_stats['messages_sent']}, "
              f"Received: {current_stats['messages_received']}")
    
    def test_server_stability(self, duration: int = 300):
        """Test server stability over time"""
        print(f"⏱️  Testing server stability for {duration}s...")
        
        start_time = time.time()
        last_check = start_time
        
        while time.time() - start_time < duration:
            time.sleep(10)  # Check every 10 seconds
            
            # Count active connections
            active_clients = sum(1 for client in self.clients if client.connected and client.authenticated)
            
            # Get system stats
            try:
                process = psutil.Process(os.getpid())
                memory_mb = process.memory_info().rss / 1024 / 1024
                cpu_percent = process.cpu_percent()
            except:
                memory_mb = 0
                cpu_percent = 0
            
            elapsed = time.time() - start_time
            print(f"📊 Stability check ({elapsed:.0f}s): Active clients: {active_clients}, "
                  f"Memory: {memory_mb:.1f}MB, CPU: {cpu_percent:.1f}%")
            
            # Randomly have some clients send messages
            active_clients_list = [c for c in self.clients if c.connected and c.authenticated]
            if active_clients_list:
                sample_size = min(50, len(active_clients_list))
                sample_clients = random.sample(active_clients_list, sample_size)
                
                for client in sample_clients:
                    if client.channels_joined:
                        channel = random.choice(list(client.channels_joined))
                        message = f"Stability test message at {elapsed:.0f}s"
                        client.send_channel_message(channel, message)
        
        print(f"🎯 Stability test completed after {duration}s")
    
    def monitor_system_resources(self):
        """Monitor system resource usage during tests"""
        def monitor():
            while True:
                try:
                    # Get system stats
                    cpu_percent = psutil.cpu_percent(interval=1)
                    memory = psutil.virtual_memory()
                    
                    # Try to get server process stats (this is approximate)
                    server_memory = 0
                    server_cpu = 0
                    for proc in psutil.process_iter(['pid', 'name', 'memory_info', 'cpu_percent']):
                        try:
                            if 'ircserv' in proc.info['name'].lower() or 'ft_irc' in proc.info['name'].lower():
                                server_memory = proc.info['memory_info'].rss / 1024 / 1024  # MB
                                server_cpu = proc.info['cpu_percent']
                                break
                        except:
                            continue
                    
                    current_stats = self.stats.get_summary()
                    print(f"🖥️  SYSTEM - CPU: {cpu_percent:.1f}%, RAM: {memory.percent:.1f}% "
                          f"| SERVER - CPU: {server_cpu:.1f}%, RAM: {server_memory:.1f}MB "
                          f"| CLIENTS: {current_stats['successful_connections']}")
                    
                    time.sleep(30)  # Update every 30 seconds
                except Exception as e:
                    print(f"Monitor error: {e}")
                    time.sleep(30)
        
        monitor_thread = threading.Thread(target=monitor, daemon=True)
        monitor_thread.start()
        return monitor_thread
    
    def cleanup(self):
        """Disconnect all clients"""
        print(f"\n🧹 Cleaning up {len(self.clients)} connections...")
        
        # Disconnect in batches to avoid overwhelming the server
        batch_size = 50
        for i in range(0, len(self.clients), batch_size):
            batch = self.clients[i:i + batch_size]
            
            with ThreadPoolExecutor(max_workers=batch_size) as executor:
                futures = [executor.submit(client.disconnect) for client in batch]
                for future in as_completed(futures):
                    try:
                        future.result(timeout=5)
                    except:
                        pass
            
            print(f"📊 Disconnected {min(i + batch_size, len(self.clients))}/{len(self.clients)} clients")
            time.sleep(0.5)  # Brief pause between batches
        
        self.clients.clear()
        print("✅ Cleanup completed")
    
    def print_final_report(self):
        """Print comprehensive performance report"""
        stats = self.stats.get_summary()
        
        print("\n" + "=" * 80)
        print("📊 PERFORMANCE TEST RESULTS")
        print("=" * 80)
        
        print(f"🕐 Total Test Duration: {stats['total_time']:.1f} seconds")
        print(f"🔗 Connection Results:")
        print(f"   ✅ Successful: {stats['successful_connections']}")
        print(f"   ❌ Failed: {stats['failed_connections']}")
        print(f"   📊 Success Rate: {stats['connection_success_rate']:.1f}%")
        print(f"   ⚡ Avg Connection Time: {stats['avg_connection_time']:.3f}s")
        print(f"   🚀 Connections/sec: {stats['connections_per_second']:.1f}")
        
        print(f"\n🏠 Channel Operations:")
        print(f"   ✅ Successful Joins: {stats['successful_joins']}")
        print(f"   ❌ Failed Joins: {stats['failed_joins']}")
        
        print(f"\n💬 Message Throughput:")
        print(f"   📤 Messages Sent: {stats['messages_sent']}")
        print(f"   📥 Messages Received: {stats['messages_received']}")
        if stats['total_time'] > 0:
            print(f"   📊 Messages/sec: {stats['messages_sent']/stats['total_time']:.1f}")
        if stats['avg_response_time'] > 0:
            print(f"   ⚡ Avg Response Time: {stats['avg_response_time']:.3f}s")
        
        print(f"\n❌ Errors: {stats['errors_count']}")
        
        # Performance assessment
        print(f"\n🎯 PERFORMANCE ASSESSMENT:")
        if stats['successful_connections'] >= 5000:
            print("🥇 EXCELLENT - Handles 5000+ concurrent connections")
        elif stats['successful_connections'] >= 1000:
            print("🥈 VERY GOOD - Handles 1000+ concurrent connections")
        elif stats['successful_connections'] >= 500:
            print("🥉 GOOD - Handles 500+ concurrent connections")
        elif stats['successful_connections'] >= 100:
            print("⚠️ MODERATE - Handles 100+ concurrent connections")
        else:
            print("❌ NEEDS IMPROVEMENT - Low connection capacity")
        
        print("=" * 80)
    
    def run_full_performance_test(self, max_users: int = 10000):
        """Run comprehensive performance test"""
        print("🚀 STARTING COMPREHENSIVE PERFORMANCE TEST")
        print(f"🎯 Target: {max_users} concurrent users")
        print(f"🖥️  Server: {self.host}:{self.port}")
        print("=" * 80)
        
        try:
            # Start system monitoring
            monitor_thread = self.monitor_system_resources()
            
            # Phase 1: Connection capacity test
            print("\n🔥 PHASE 1: Connection Capacity Test")
            self.test_connection_capacity(max_users, batch_size=100, delay_between_batches=0.2)
            
            if len(self.clients) < 10:
                print("❌ Too few clients connected. Stopping test.")
                return
            
            time.sleep(5)  # Let connections stabilize
            
            # Phase 2: Channel operations test
            print("\n🔥 PHASE 2: Channel Operations Test")
            self.test_channel_operations()
            
            time.sleep(5)
            
            # Phase 3: Message throughput test
            print("\n🔥 PHASE 3: Message Throughput Test")
            self.test_message_throughput(duration=120, messages_per_client=5)
            
            time.sleep(5)
            
            # Phase 4: Stability test
            print("\n🔥 PHASE 4: Server Stability Test")
            self.test_server_stability(duration=180)  # 3 minutes
            
        except KeyboardInterrupt:
            print("\n⚠️ Test interrupted by user")
        except Exception as e:
            print(f"\n❌ Test failed with error: {e}")
        finally:
            self.cleanup()
            self.print_final_report()

def main():
    parser = argparse.ArgumentParser(description='IRC Server Performance Testing Tool')
    parser.add_argument('--host', default='localhost', help='IRC server host')
    parser.add_argument('--port', type=int, default=6667, help='IRC server port')
    parser.add_argument('--password', help='Server password')
    parser.add_argument('--users', type=int, default=1000, help='Number of users to simulate (default: 1000, max recommended: 10000)')
    parser.add_argument('--quick', action='store_true', help='Run quick test (fewer users, shorter duration)')
    
    args = parser.parse_args()
    
    if args.quick:
        max_users = min(args.users, 100)
        print("🚀 Running QUICK performance test")
    else:
        max_users = args.users
        print("🚀 Running FULL performance test")
    
    print(f"⚠️  WARNING: This test will create {max_users} concurrent connections!")
    print("⚠️  Make sure your system has enough file descriptors and memory.")
    print("⚠️  Press Ctrl+C to stop the test at any time.")
    
    # Ask for confirmation for large tests
    if max_users > 1000:
        response = input(f"\n🤔 Are you sure you want to test with {max_users} users? (y/N): ")
        if response.lower() not in ['y', 'yes']:
            print("Test cancelled.")
            return
    
    # Increase file descriptor limit if possible
    try:
        import resource
        soft, hard = resource.getrlimit(resource.RLIMIT_NOFILE)
        if soft < max_users + 100:
            resource.setrlimit(resource.RLIMIT_NOFILE, (min(hard, max_users + 100), hard))
            print(f"📊 Increased file descriptor limit to {min(hard, max_users + 100)}")
    except:
        print("⚠️ Could not increase file descriptor limit. Test may fail with many connections.")
    
    tester = IRCPerformanceTester(args.host, args.port, args.password)
    tester.run_full_performance_test(max_users)

if __name__ == "__main__":
    main()