#!/usr/bin/python

from mininet.topo import Topo
from mininet.net import Mininet
from mininet.node import UserSwitch
from mininet.link import TCLink
from mininet.util import dumpNodeConnections
from mininet.log import setLogLevel

class GthTopo(Topo):
	"gth topology created from lgf file."

	def __init__(self, **opts):
		# Initialize topology and default options
		Topo.__init__(self, **opts)

		s4 = self.addSwitch('s4',cls=UserSwitch);
		s3 = self.addSwitch('s3',cls=UserSwitch);
		s2 = self.addSwitch('s2',cls=UserSwitch);
		s1 = self.addSwitch('s1',cls=UserSwitch);

		self.addLink(s4, s3, port1=2, port2=2, cls=TCLink, **dict(bw=1, delay='0ms', loss=0, max_queue_size=1000, use_htb=True));
		self.addLink(s4, s2, port1=3, port2=2, cls=TCLink, **dict(bw=1, delay='0ms', loss=0, max_queue_size=1000, use_htb=True));
		self.addLink(s2, s3, port1=3, port2=3, cls=TCLink, **dict(bw=1, delay='0ms', loss=0, max_queue_size=1000, use_htb=True));
		self.addLink(s1, s2, port1=2, port2=4, cls=TCLink, **dict(bw=1, delay='0ms', loss=0, max_queue_size=1000, use_htb=True));

		h1 = self.addHost('h1')
		h3 = self.addHost('h3')
		h4 = self.addHost('h4')

		self.addLink(h1, s1, port1=0, port2=1, cls=TCLink);
		self.addLink(h3, s3, port1=0, port2=1, cls=TCLink);
		self.addLink(h4, s4, port1=0, port2=1, cls=TCLink);

topos={'gth':(lambda: GthTopo())};

