#!/usr/bin/env python3

"""
PhantomVault IPC Client Test Script
Tests the IPC communication between GUI and service
"""

import socket
import json
import struct
import sys
import os
import time

class PhantomVaultIPCClient:
    def __init__(self, socket_path=None):
        if socket_path is None:
            socket_path = f"/tmp/phantom-vault-{os.getuid()}.sock"
        self.socket_path = socket_path
        self.socket = None
        self.client_id = f"test_client_{int(time.time())}"
    
    def connect(self):
        """Connect to the PhantomVault service"""
        try:
            self.socket = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
            self.socket.connect(self.socket_path)
            print(f"✅ Connected to PhantomVault service at {self.socket_path}")
            return True
        except Exception as e:
            print(f"❌ Failed to connect: {e}")
            return False
    
    def disconnect(self):
        """Disconnect from the service"""
        if self.socket:
            self.socket.close()
            self.socket = None
            print("✅ Disconnected from service")
    
    def send_message(self, message_type, payload="", request_id=""):
        """Send a message to the service"""
        if not self.socket:
            print("❌ Not connected to service")
            return None
        
        message = {
            "type": message_type,
            "payload": payload,
            "request_id": request_id or f"req_{int(time.time() * 1000)}",
            "client_id": self.client_id
        }
        
        try:
            # Serialize message
            serialized = json.dumps(message)
            message_bytes = serialized.encode('utf-8')
            
            # Send message length first
            length = struct.pack('I', len(message_bytes))
            self.socket.send(length)
            
            # Send message data
            self.socket.send(message_bytes)
            
            print(f"📤 Sent {message_type}: {payload[:50]}{'...' if len(payload) > 50 else ''}")
            
            # Receive response
            return self.receive_message()
            
        except Exception as e:
            print(f"❌ Failed to send message: {e}")
            return None
    
    def receive_message(self):
        """Receive a message from the service"""
        if not self.socket:
            return None
        
        try:
            # Receive message length
            length_data = self.socket.recv(4)
            if len(length_data) != 4:
                return None
            
            message_length = struct.unpack('I', length_data)[0]
            
            # Receive message data
            message_data = b""
            while len(message_data) < message_length:
                chunk = self.socket.recv(message_length - len(message_data))
                if not chunk:
                    break
                message_data += chunk
            
            if len(message_data) != message_length:
                print(f"❌ Incomplete message received: {len(message_data)}/{message_length}")
                return None
            
            # Deserialize message
            message = json.loads(message_data.decode('utf-8'))
            print(f"📥 Received {message['type']}: {message['payload'][:50]}{'...' if len(message['payload']) > 50 else ''}")
            
            return message
            
        except Exception as e:
            print(f"❌ Failed to receive message: {e}")
            return None

def test_ipc_communication():
    """Test IPC communication with PhantomVault service"""
    print("🔧 PhantomVault IPC Client Test")
    print("=" * 40)
    
    client = PhantomVaultIPCClient()
    
    # Test 1: Connection
    print("\n📡 Test 1: Connection")
    if not client.connect():
        print("❌ Connection test failed")
        return False
    
    # Test 2: Ping
    print("\n🏓 Test 2: Ping")
    response = client.send_message("PING", "ping")
    if response and response['type'] == 'PONG':
        print("✅ Ping test passed")
    else:
        print("❌ Ping test failed")
    
    # Test 3: Get Vault State
    print("\n🏦 Test 3: Get Vault State")
    response = client.send_message("GET_VAULT_STATE")
    if response and response['type'] == 'VAULT_STATE_UPDATE':
        print("✅ Vault state test passed")
        try:
            state = json.loads(response['payload'])
            print(f"   Service running: {state.get('service_running', 'unknown')}")
            print(f"   Uptime: {state.get('uptime_seconds', 0)} seconds")
            print(f"   Active profile: {state.get('active_profile', 'none')}")
            print(f"   Folders: {len(state.get('folders', []))}")
        except:
            print("   (Could not parse state details)")
    else:
        print("❌ Vault state test failed")
    
    # Test 4: Get Profiles
    print("\n👤 Test 4: Get Profiles")
    response = client.send_message("GET_PROFILES")
    if response and response['type'] == 'PROFILE_UPDATE':
        print("✅ Profiles test passed")
        try:
            profiles_data = json.loads(response['payload'])
            profiles = profiles_data.get('profiles', [])
            print(f"   Found {len(profiles)} profile(s)")
            for profile in profiles:
                print(f"   - {profile.get('name', 'Unknown')} (ID: {profile.get('id', 'N/A')})")
        except:
            print("   (Could not parse profile details)")
    else:
        print("❌ Profiles test failed")
    
    # Test 5: Invalid Message
    print("\n❓ Test 5: Invalid Message")
    response = client.send_message("INVALID_MESSAGE_TYPE", "test")
    if response and response['type'] == 'ERROR_NOTIFICATION':
        print("✅ Invalid message handling test passed")
    else:
        print("⚠️  Invalid message handling test - no error response")
    
    # Cleanup
    print("\n🧹 Cleanup")
    client.disconnect()
    
    print("\n✅ IPC communication test completed")
    return True

def main():
    """Main function"""
    if len(sys.argv) > 1:
        if sys.argv[1] == "--help":
            print("PhantomVault IPC Client Test")
            print("Usage: python3 test-ipc-client.py [socket_path]")
            print("")
            print("Tests IPC communication with PhantomVault service")
            print("Default socket path: /tmp/phantom-vault-{uid}.sock")
            return
        
        # Custom socket path provided
        socket_path = sys.argv[1]
        client = PhantomVaultIPCClient(socket_path)
    else:
        client = PhantomVaultIPCClient()
    
    # Check if service is running
    socket_path = client.socket_path
    if not os.path.exists(socket_path):
        print(f"❌ Socket file not found: {socket_path}")
        print("Make sure PhantomVault service is running:")
        print("  ./scripts/manage-service.sh status")
        return
    
    # Run tests
    try:
        test_ipc_communication()
    except KeyboardInterrupt:
        print("\n\n⚠️  Test interrupted by user")
    except Exception as e:
        print(f"\n❌ Test failed with exception: {e}")

if __name__ == "__main__":
    main()