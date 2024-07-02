import socket
import time
import struct

MESSAGE_JOIN = 0
MESSAGE_WELCOME = 1
MESSAGE_ACK = 2
MESSAGE_POSITION = 3
MESSAGE_MELEE = 4

class Player:
    def __init__(self, player_id, address_and_port):
        self.player_id = player_id
        self.address_and_port = address_and_port
        self.x = 0
        self.y = 0
        self.movement_order = 0
        self.reliable_messages = {}
        #{MessageID : (Timestamp , Content)}
        self.karma = 3


class Server:
    def __init__(self, address, port):
        self.address = address
        self.port = port
        self.socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self.socket.bind((self.address, self.port))
        self.players = {}
        self.format = "<IIIff"
        #format I - message_type | I - message_counter | I - player_id | ff - Position
        # 0 - Join \ 1 - Welcome \ 2 - Position \ 3 - Melee
        self.message_counter = 0
        self.player_counter = 100
        self.blacklist = {}
        #{(ip, port) : Timestamp}

    def send_raw_message(self, destination, payload):
        self.socket.sendto(payload, destination)

    def send_message(self, destination, message_type, payload):
        header = struct.pack("<II", message_type, self.message_counter)
        self.send_raw_message(destination, header + payload)
        self.message_counter += 1

        return header + payload, self.message_counter - 1
    
    def send_reliable_message(self, destination, message_type, payload):
        message, message_id = self.send_message(destination, message_type, payload)
        self.players[destination].reliable_messages[message_id] = (time.time(), message)

    def decrease_karma(self, player, amount):
        self.players[player].karma -= amount

        if self.players[player].karma <= 0:

            del(self.players[player])
            self.blacklist[player] = time.time()

    def check_reliable_messages(self):
        
        currrent_time = time.time()

        for player in self.players:
            for message_id in self.players[player].reliable_messages:
                reliable_message = self.players[player].reliable_messages[message_id]
                time_stamp, payload = reliable_message

                delta_time = currrent_time - time_stamp

                if delta_time >= 3.0:
                    self.send_raw_message(player, payload)
                    self.players[player].reliable_messages[message_id] = (currrent_time, payload)


                

    def tick(self):

        self.check_reliable_messages()

        data, sender = self.socket.recvfrom(4096)

        if len(data) < 8:
            print("ignoring broken packet from", sender)
            return
        
        message_type, packet_id = struct.unpack("<II", data[:8])

        if message_type == MESSAGE_JOIN:

            if sender in self.players:
                self.decrease_karma(sender, 1)

            else:
                self.player_counter += 1
                player_id = self.player_counter + 1
                new_player = Player(player_id, sender)
                self.players[sender] = new_player
                print("new player", sender, player_id)

                payload = struct.pack("<I", player_id)
                self.send_reliable_message(sender, MESSAGE_WELCOME, payload)

                return

        elif message_type == MESSAGE_ACK:
            
            if len(data) != 12:
                print("Ack not right size", sender)
                self.decrease_karma(sender, 2)
                return
            
            reliable_message_id, = struct.unpack("<I", data[8:12])

            if reliable_message_id not in self.players[sender].reliable_messages:
                self.decrease_karma(sender, 1)
                return
            
            del(self.players[sender].reliable_messages[reliable_message_id])

        elif message_type == MESSAGE_POSITION:
            
            if len(data) != 20:
                self.decrease_karma(sender, 1)
                return
            
            player_id, x, y = struct.unpack("<Iff", data[8:20])

            if self.players[sender].player_id != player_id:
                #If player id != sender -> Move another player
                print("invalid player_id for", sender)
                self.decrease_karma(sender, 5)
                return

            if packet_id <= self.players[sender].movement_order:
                return
            
            self.players[sender].x = x
            self.players[sender].y = y

            self.players[sender].movement_order = packet_id

            print(x, y, "from", sender, "player_id", player_id)

            position_packet = struct.pack("<Iff", player_id, x, y)

            for player in self.players:
                if player == sender:
                    continue
                self.send_message(player, MESSAGE_POSITION, position_packet)

    def run(self):
        while True:
            self.tick()


if __name__ == "__main__":
    server = Server("192.168.1.13", 9999)
    server.run()
