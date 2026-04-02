// SPDX-License-Identifier: MIT
pragma solidity ^0.8.20;

contract ProvenanceGatekeeper {
    struct Artifact {
        bytes32 fileHash;
        uint256 timestamp;
        address registeredBy;
    }

    mapping(string => Artifact) public registry;
    event ProvenanceLogged(string artifactName, bytes32 fileHash, uint256 timestamp);

    function registerArtifact(string memory _name, bytes32 _hash) public {
        registry[_name] = Artifact({
            fileHash: _hash,
            timestamp: block.timestamp,
            registeredBy: msg.sender
        });
        emit ProvenanceLogged(_name, _hash, block.timestamp);
    }

    function verifyProvenance(string memory _name, bytes32 _currentHash) public view returns (bool) {
        return registry[_name].fileHash == _currentHash;
    }
}
