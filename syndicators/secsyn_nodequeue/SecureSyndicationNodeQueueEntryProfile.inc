<?php

class SecureSyndicationNodeQueueEntryProfile extends SecureSyndicationProfile {

  public function dependencies($object) {
    $dependencies = array();
    $queue = db_select('nodequeue_subqueue', 'sq');
    $queue->fields('sq');
    $queue->condition('sqid', $object->sqid);
    $rs = $queue->execute();
    $obj = $rs->fetchObject();
    if (!empty($obj)) {
      $dependencies[] = array(
        'profile' => 'SecureSyndicationSubNodeQueueProfile',
        'object' => $obj,
      );
    }
    return $dependencies;
  }

  public function package($object) {
    $package = (array) $object;
    unset($package['qid']);
    unset($package['sqid']);
    unset($package['nid']);
    return $package;
  }

  public function update($uuid, $fields) {
    $data = unserialize($uuid);
    $node = reset(entity_uuid_load('node', array($data['node_uuid'])));
    $queue = nodequeue_load_queue_by_name($data['queue_name']);
    $subqueue = secsyn_nodequeue_load_subqueue_by_reference($data['queue_name'], $data['subqueue_reference']);
    db_merge('nodequeue_nodes')
        ->fields($fields + array(
          'nid' => $node->nid,
          'qid' => $queue->qid,
          'sqid' => $subqueue->qid,
        ))
        ->key(array(
          'nid' => $node->nid,
          'qid' => $queue->qid,
          'sqid' => $subqueue->qid,
        ))
        ->execute();
  }

  public function lookup($uuid) {
    $data = unserialize($uuid);
    $nodes = entity_uuid_load('node', array($data['node_uuid']));
    $node = reset($nodes);
    if (empty($node)) {
      return FALSE;
    }
    $queue = nodequeue_load_queue_by_name($data['queue_name']);
    if (empty($queue)) {
      return FALSE;
    }
    $subqueue = secsyn_nodequeue_load_subqueue_by_reference($queue->qid, $data['subqueue_reference']);
    if (empty($subqueue)) {
      return FALSE;
    }
    return db_select('nodequeue_nodes', 'nqn')
        ->fields('nqn')
        ->condition('nid', $node->nid)
        ->condition('sqid', $subqueue->sqid)
        ->condition('qid', $queue->qid)
        ->execute()
        ->fetchObject();
  }

  public function uuid($object) {
    $uuid = array();
    $queue = nodequeue_load($object->qid);
    $subqueue = db_select('nodequeue_subqueue', 'sq')
        ->fields('sq')
        ->condition('qid', $object->qid)
        ->condition('sqid', $object->sqid)
        ->execute()
        ->fetch();
    $node = node_load($object->nid);
    $uuid['queue_name'] = $queue->name;
    $uuid['subqueue_reference'] = $subqueue->reference;
    $uuid['node_uuid'] = $node->uuid;
    return serialize($uuid);
  }

}