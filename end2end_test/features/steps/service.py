import docker
import socket


class Node:
    Network = 'cdcf_end2end_test'
    Port = 3335

    @staticmethod
    def id(name):
        return f"{name}:4445"

    def __init__(self, name, seeds):
        self.name, self.seeds, self.container = name, seeds, None

    def start(self):
        if self.container:
            self.container.start()
            return True
        else:
            return self.run()

    def run(self):
        client = docker.from_env()
        try:
            client.networks.get(Node.Network)
        except docker.errors.NotFound as e:
            client.networks.create(Node.Network)
        self.port = Node.Port + 1
        Node.Port = self.port
        self.container = client.containers.run('cdcf', detach=True, network=Node.Network,
                                               name=self.name,
                                               environment=[f"HOST={self.name}", f"SEEDS={self.seeds}"],
                                               ports={'3335/tcp': self.port})
        return True

    def members(self):
        host = socket.gethostname()
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        s.connect((host, self.port))
        s.settimeout(5)
        s.sendall(b"Hello, world\n")
        s.settimeout(5)
        data = s.recv(1024).decode('utf-8')
        s.close()
        results = data.split('\n')
        results = [line for line in results if line]
        return results

    def stop(self):
        if self.running():
            self.container.kill()

    def remove(self):
        if self.container is not None:
            self.container.remove()

    def running(self):
        if self.container is not None:
            self.container.reload()
            return self.container.status != 'exited'
        return False

    def log(self):
        if self.container is not None:
            logs = self.container.logs()
            print(f"{self.name} log:\n==============\n", logs.decode('utf-8'))

    def disconnect(self):
        client = docker.from_env()
        network = client.networks.get(Node.Network)
        network.disconnect(self.container)


def configure_node(name, seeds):
    return Node(name, seeds)
