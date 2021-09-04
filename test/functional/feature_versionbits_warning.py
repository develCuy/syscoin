#!/usr/bin/env python3
# Copyright (c) 2016-2020 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test version bits warning system.

Generate chains with block versions that appear to be signalling unknown
soft-forks, and test that warning alerts are generated.
"""
import os
import re

from test_framework.blocktools import create_block, create_coinbase
from test_framework.messages import msg_block
from test_framework.p2p import P2PInterface
from test_framework.test_framework import SyscoinTestFramework

VB_PERIOD = 144           # versionbits period length for regtest
VB_THRESHOLD = 108        # versionbits activation threshold for regtest
VB_TOP_BITS = 0x20000000
# SYSCOIN
VB_UNKNOWN_BIT = 15       # Choose a bit unassigned to any deployment, can't be above bit 16 because of auxpow
VB_UNKNOWN_VERSION = VB_TOP_BITS | (1 << VB_UNKNOWN_BIT)

WARN_UNKNOWN_RULES_ACTIVE = f"Unknown new rules activated (versionbit {VB_UNKNOWN_BIT})"
VB_PATTERN = re.compile("Unknown new rules activated.*versionbit")

class VersionBitsWarningTest(SyscoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 1

    def setup_network(self):
        self.alert_filename = os.path.join(self.options.tmpdir, "alert.txt")
        # Open and close to create zero-length file
        with open(self.alert_filename, 'w', encoding='utf8'):
            pass
        self.extra_args = [[f"-alertnotify=echo %s >> \"{self.alert_filename}\""]]
        self.setup_nodes()

    def send_blocks_with_version(self, peer, numblocks, version):
        """Send numblocks blocks to peer with version set"""
        tip = self.nodes[0].getbestblockhash()
        height = self.nodes[0].getblockcount()
        block_time = self.nodes[0].getblockheader(tip)["time"] + 1
        tip = int(tip, 16)

        for _ in range(numblocks):
            block = create_block(tip, create_coinbase(height + 1), block_time)
            block.nVersion = version
            block.solve()
            peer.send_message(msg_block(block))
            block_time += 1
            height += 1
            tip = block.sha256
        peer.sync_with_ping()

    def versionbits_in_alert_file(self):
        """Test that the versionbits warning has been written to the alert file."""
        alert_text = open(self.alert_filename, 'r', encoding='utf8').read()
        return VB_PATTERN.search(alert_text) is not None

    def run_test(self):
        node = self.nodes[0]
        peer = node.add_p2p_connection(P2PInterface())

        node_deterministic_address = node.get_deterministic_priv_key().address
        # Mine one period worth of blocks
        node.generatetoaddress(VB_PERIOD, node_deterministic_address)

        self.log.info("Check that there is no warning if previous VB_BLOCKS have <VB_THRESHOLD blocks with unknown versionbits version.")
        # Build one period of blocks with < VB_THRESHOLD blocks signaling some unknown bit
        self.send_blocks_with_version(peer, VB_THRESHOLD - 1, VB_UNKNOWN_VERSION)
        node.generatetoaddress(VB_PERIOD - VB_THRESHOLD + 1, node_deterministic_address)

        # Check that we're not getting any versionbit-related errors in get*info()
        assert not VB_PATTERN.match(node.getmininginfo()["warnings"])
        assert not VB_PATTERN.match(node.getnetworkinfo()["warnings"])

        # Build one period of blocks with VB_THRESHOLD blocks signaling some unknown bit
        self.send_blocks_with_version(peer, VB_THRESHOLD, VB_UNKNOWN_VERSION)
        node.generatetoaddress(VB_PERIOD - VB_THRESHOLD, node_deterministic_address)

        self.log.info("Check that there is a warning if previous VB_BLOCKS have >=VB_THRESHOLD blocks with unknown versionbits version.")
        # Mine a period worth of expected blocks so the generic block-version warning
        # is cleared. This will move the versionbit state to ACTIVE.
        node.generatetoaddress(VB_PERIOD, node_deterministic_address)

        # Stop-start the node. This is required because syscoind will only warn once about unknown versions or unknown rules activating.
        self.restart_node(0)

        # Generating one block guarantees that we'll get out of IBD
        node.generatetoaddress(1, node_deterministic_address)
        self.wait_until(lambda: not node.getblockchaininfo()['initialblockdownload'])
        # Generating one more block will be enough to generate an error.
        node.generatetoaddress(1, node_deterministic_address)

        assert WARN_UNKNOWN_RULES_ACTIVE in node.getmininginfo()["warnings"]
        assert WARN_UNKNOWN_RULES_ACTIVE in node.getnetworkinfo()["warnings"]
        # Check that the alert file shows the versionbits unknown rules warning
        self.wait_until(lambda: self.versionbits_in_alert_file())

if __name__ == '__main__':
    VersionBitsWarningTest().main()
