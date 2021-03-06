<?php

class SecureSyndicationSubNodeQueueProfile extends SecureSyndicationProfile {

  public function dependencies($object) {
    $queue = nodequeue_load($object->qid);
    return array(
      array(
        'profile' => 'SecureSyndicationNodeQueueProfile',
        'object' => $queue,
      ),
    );
  }

  public function package($object) {
    $package = (array) $object;
    $queue = nodequeue_load($package['qid']);
    $package['queue_name'] = $queue->name;
    unset($package['qid']);
    unset($package['sqid']);
    return $package;
  }

  public function update($uuid, $fields) {
    $queue = nodequeue_load_queue_by_name($fields['queue_name']);
    $fields['qid'] = $queue->qid;
    unset($fields['queue_name']);
    $existing = $this->lookup($uuid);
    if (!empty($existing)) {
      $fields['sqid'] = $existing->sqid;
    } else {
    }
    db_merge('nodequeue_subqueue')
        ->fields($fields)
        ->key(array(
          'sqid' => isset($fields['sqid']) ? $fields['sqid'] : NULL,
        ))
        ->execute();
  }

  public function lookup($uuid) {
    $pieces = explode('/', $uuid, 2);
    $queue = nodequeue_load_queue_by_name($pieces[0]);
    if (empty($queue)) {
      return FALSE;
    }
    return (object) secsyn_nodequeue_load_subqueue_by_reference($queue->qid, $pieces[1]);
  }

  public function uuid($object) {
    $queue = nodequeue_load($object->qid);
    return $queue->name . '/' . $object->reference;
  }

}