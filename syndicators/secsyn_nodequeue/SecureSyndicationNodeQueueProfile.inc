<?php

class SecureSyndicationNodeQueueProfile extends SecureSyndicationProfile {

  public function children($object) {
    $children = array();
    $rs = db_select('nodequeue_subqueue', 'sq')
        ->fields('sq')
        ->condition('qid', $object->qid)
        ->execute();
    foreach ($rs as $row) {
      $children[] = array(
        'object' => $row,
        'profile' => 'SecureSyndicationSubNodeQueueProfile',
      );
    }
    return $children;
  }

  public function package($object) {
    $package = (array) $object;
    unset($package['qid']);
    return $package;
  }

  public function update($uuid, $fields) {
    $existing = $this->lookup($uuid);
    if ($existing) {
      $fields['name'] = $uuid;
      $fields['qid'] = $existing->qid;
    }
    nodequeue_save((object) $fields);
  }

  public function lookup($uuid) {
    return nodequeue_load_queue_by_name($uuid);
  }

  public function uuid($object) {
    return $object->name;
  }

}
