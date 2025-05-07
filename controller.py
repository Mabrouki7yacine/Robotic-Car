import socket
import struct
import pygame
import threading
import time
import queue

# Initialize data and communication queue
data = {
    "left_direction": 0,
    "left_speed": 0,
    "right_direction": 0,
    "right_speed": 0
}

# Queue for passing data between threads
data_queue = queue.Queue()
connected = False
client_socket = None
prev_data = data.copy()

def tcp_connection():
    global data, connected, client_socket
    print("Hello waiting for connection")
    server_address = ("", 8000)
    server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    server_socket.bind(server_address)
    server_socket.listen(4) 

    while True:
        try:
            print("Waiting for client connection...")
            client_socket, client_address = server_socket.accept() 
            print("Connection from:", client_address)
            connected = True
            
            # Process data from queue and send over TCP
            while connected:
                try:
                    # Get new data from queue if available, otherwise use last known data
                    try:
                        new_data = data_queue.get(block=False)
                        current_data = new_data
                    except queue.Empty:
                        # No new data, use current global data
                        current_data = data
                    
                    # Send the data
                    packed_data = struct.pack('4i', 
                                             current_data["right_direction"], 
                                             current_data["left_direction"], 
                                             current_data["right_speed"], 
                                             current_data["left_speed"])
                    client_socket.sendall(packed_data)
                    print(f"Sent: {current_data}")
                    
                    # Short delay to prevent overwhelming the socket
                    time.sleep(0.1)
                    
                except ConnectionResetError:
                    print("Connection reset by peer")
                    connected = False
                    break
                except BrokenPipeError:
                    print("Broken pipe")
                    connected = False
                    break
                
            # Close socket on disconnect
            if client_socket:
                client_socket.close()
                client_socket = None
                
        except KeyboardInterrupt:
            print("Server stopped by user")
            if client_socket:
                client_socket.close()
            server_socket.close()
            break
        except Exception as e:
            print(f"TCP error: {e}")
            if client_socket:
                client_socket.close()
                client_socket = None
            time.sleep(1)  # Wait before trying to accept again

def handle_controller():
    speed = 100
    global data, prev_data
    
    # Initialize pygame properly
    pygame.init()
    
    # Initialize the joystick module specifically
    pygame.joystick.init()
    
    # Check if any joysticks/controllers are connected
    joystick_count = pygame.joystick.get_count()
    if joystick_count == 0:
        print("No joystick detected. Please connect a controller.")
        return
    
    # Initialize the first joystick
    controller = pygame.joystick.Joystick(0)
    controller.init()
    print(f"Controller Name: {controller.get_name()}")
    
    try:
        while True:
            data_changed = False
            pygame.event.pump()  # Process any events
            
            # Store previous data for comparison
            prev_data = data.copy()
            
            for event in pygame.event.get():
                if event.type == pygame.JOYAXISMOTION:
                    axe = int(controller.get_axis(1) * -10)
                    right_left = int(controller.get_axis(0) * 10)
                    data["right_speed"] = data["left_speed"] = speed 

                    if (axe == 0 and right_left == 0):
                        data["right_direction"] = data["left_direction"] = 0
                        data["right_speed"] = data["left_speed"] = 0   
                    elif (axe > 3):  # Forward
                        data["left_direction"], data["right_direction"] = 1, 1
                    elif (axe < -3):  # Backward
                        data["left_direction"], data["right_direction"] = -1, -1
                    elif (right_left > 3):  # Right turn
                        data["left_direction"], data["right_direction"] = 1, 0
                        data["right_speed"], data["left_speed"] = speed, speed 
                    elif (right_left < -3):  # Left turn
                        data["left_direction"], data["right_direction"] = 0, 1
                        data["right_speed"], data["left_speed"] = speed, speed 
                    
                    data_changed = True

                elif event.type == pygame.JOYBUTTONUP:
                    if (event.button == 0):  # Decrease speed
                        speed = speed - 25
                        speed = max(0, min(speed, 255))
                        print(f"Speed decreased to: {speed}")
                        
                        # Update speed in data if motors are running
                        if data["right_direction"] != 0 or data["left_direction"] != 0:
                            data["right_speed"] = data["left_speed"] = speed
                            data_changed = True
                            
                    elif (event.button == 3):  # Increase speed
                        speed = speed + 25
                        speed = max(0, min(speed, 255))
                        print(f"Speed increased to: {speed}")
                        
                        # Update speed in data if motors are running
                        if data["right_direction"] != 0 or data["left_direction"] != 0:
                            data["right_speed"] = data["left_speed"] = speed
                            data_changed = True
            
            # Check for changes in data
            if data_changed and connected:
                if data != prev_data:  # Only send if data has changed
                    print("Data changed, queuing for sending")
                    data_queue.put(data.copy())  # Put a copy of data in the queue
            
            # Sample joystick state regularly in case no events occurred
            axis_1 = int(controller.get_axis(1) * -10)
            axis_0 = int(controller.get_axis(0) * 10)
            
            # Check for joystick state when no events are triggered
            # This handles the case where joystick returns to center without events
            if axis_1 == 0 and axis_0 == 0 and (data["right_direction"] != 0 or data["left_direction"] != 0):
                data["right_direction"] = data["left_direction"] = 0
                data["right_speed"] = data["left_speed"] = 0
                data_changed = True
                data_queue.put(data.copy())
                
            # Short sleep to prevent CPU hogging
            time.sleep(0.01)
            
    except Exception as e:
        print(f"Controller error: {e}")
        pygame.joystick.quit()

def keyboard_input():
    """Handle keyboard input for manual control and quitting"""
    while True:
        try:
            cmd = input("Enter command (q to quit): ")
            if cmd.lower() in ["q", "quit", "exit"]:
                print("Exiting program...")
                pygame.quit()
                if client_socket:
                    client_socket.close()
                exit(0)
        except KeyboardInterrupt:
            print("Exiting program...")
            pygame.quit()
            if client_socket:
                client_socket.close()
            exit(0)

if __name__ == '__main__':
    print("Starting robot control server...")
    
    # Start controller thread
    controller_thread = threading.Thread(target=handle_controller)
    controller_thread.daemon = True
    controller_thread.start()
    
    # Start TCP server thread
    sender_thread = threading.Thread(target=tcp_connection)
    sender_thread.daemon = True
    sender_thread.start()
    
    # Start keyboard input thread
    keyboard_thread = threading.Thread(target=keyboard_input)
    keyboard_thread.daemon = True
    keyboard_thread.start()
    
    # Keep main thread alive
    try:
        while True:
            time.sleep(1)
    except KeyboardInterrupt:
        print("Exiting program")
        pygame.quit()
        exit(0)