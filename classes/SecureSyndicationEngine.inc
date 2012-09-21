<?php

class SecureSyndicationEngine {

  private $check_queue = NULL;
  private $syndication_queue = NULL;
  private $uuids_checked = array();

  public function __construct() {

  }

  public function validationQueueSize() {
    if (empty($this->check_queue)) {
      $this->check_queue = UniqueQueue::get('secsyn_check');
    }
    return $this->check_queue->itemsLeft();
  }

  public function syndicationQueueSize() {
    if (empty($this->syndication_queue)) {
      $this->syndication_queue = UniqueQueue::get('secsyn_syndication');
    }
    return $this->syndication_queue->itemsLeft();
  }

  public function queue($remote_server, $synprofile_name, $object) {
    $profile = SecureSyndicationProfile::getProfile($synprofile_name);
    if (empty($profile)) {
      drupal_set_message(t('Unable to queue object, invalid profile: !profile', array('!profile' => $synprofile_name)), 'error');
      return FALSE;
    }
    $uuid = $profile->uuid($object);
    if (empty($uuid)) {
      drupal_set_message(t('Unable to queue object, object type does not support UUIDs'), 'error');
      return FALSE;
    }
    $key = $synprofile_name . '/' . $uuid;
    if (!in_array($key, $this->uuids_checked)) {
      $this->uuids_checked[] = $key;
      drupal_set_message(t('Queuing @uuid for syndication validation', array('@uuid' => $uuid)));
      $dependencies = $profile->dependencies($profile->lookup($uuid));
      foreach ($dependencies as $dependency) {
        if (!$this->queue($remote_server, $dependency['profile'], $dependency['object'])) {
          drupal_set_message(t('Unable to queue dependency'), 'error');
          return FALSE;
        }
      }
      $data = array(
        'unique_id' => $uuid,
        'remote_server' => $remote_server,
        'profile' => $synprofile_name,
      );
      if (empty($this->check_queue)) {
        $this->check_queue = UniqueQueue::get('secsyn_check');
      }
      return $this->check_queue->createItem($data, serialize($data));
    } else {
      return TRUE;
    }
  }

  public function checkForUpdates($proposed_updates) {
    $update_uuids = array();
    foreach ($proposed_updates as $update) {
      $update = (array) $update;
      // make sure the profile is installed on this machine
      if (class_exists($update['profile'])) {
        $profile = SecureSyndicationProfile::getProfile($update['profile']);
        // make sure it's actually an implementation of SyndicationProfile
        if ($profile) {
          // make sure the versions match
          if ($profile->version() == $update['version']) {
            $object = $profile->lookup($update['uuid']);
            // if the object does exist...
            if (!empty($object)) {
              // we only want to update the objects if the digest is different
              $digest = $profile->hash($object, $update['lookup_fields']);
              if ($digest != $update['digest']) {
                $update_uuids[] = $update['uuid'];
              }
            }
            // if the object doesn't exist, we want to update it all the time
            else {
              $update_uuids[] = $update['uuid'];
            }
          }
        }
      }
    }
    // this should now be a list of UUIDs that need updating on this server
    return $update_uuids;
  }

  public function checkNextBatch($batch_size = 250) {
    if (empty($this->check_queue)) {
      $this->check_queue = UniqueQueue::get('secsyn_check');
    }
    $batch = array();
    $original_items = array();
    for ($k = 0; $k < $batch_size; $k++) {
      $nextItem = $this->check_queue->claimItem(60);
      if (!empty($nextItem)) {
        $original_items[] = $nextItem;
        if (!isset($batch[$nextItem->data['remote_server']])) {
          $batch[$nextItem->data['remote_server']] = array();
        }
        $profile = SecureSyndicationProfile::getProfile($nextItem->data['profile']);
        $hash_results = $profile->hash($profile->lookup($nextItem->data['unique_id']));
        $batch[$nextItem->data['remote_server']][$nextItem->data['unique_id']] = array(
          'profile' => $nextItem->data['profile'],
          'version' => $profile->version(),
          'uuid' => $nextItem->data['unique_id'],
          'digest' => $hash_results['hash'],
          'lookup_fields' => $hash_results['lookup_fields'],
        );
      } else {
        $k = $batch_size;
      }
    }
    if (empty($original_items)) {
      return NULL;
    }
    foreach ($batch as $remote_server => $items) {
      module_load_include('inc', 'secsyn', 'services/secsyn.check');
      $results = secsyn_service_invoke_check($remote_server, $items);
      foreach ($results as $result_id) {
        $this->queueForSyndication($remote_server, $items[$result_id]['profile'], $items[$result_id]['uuid']);
      }
    }
    foreach ($original_items as $item) {
      $this->check_queue->deleteItem($item);
    }
    return TRUE;
  }

  public function queueForSyndication($remote_server, $profile, $uuid) {
    drupal_set_message(t('Queuing !uuid for syndication', array('!uuid' => $uuid)));
    $data = array(
      'remote_server' => $remote_server,
      'profile' => $profile,
      'uuid' => $uuid,
    );
    if (empty($this->syndication_queue)) {
      $this->syndication_queue = UniqueQueue::get('secsyn_syndication');
    }
    $this->syndication_queue->createItem($data, serialize($data));
  }

  public function executeUpdates($updates) {
    $success = array();
    foreach ($updates as $update) {
      $update = (array) $update;
      if (class_exists($update['profile'])) {
        $profile = SecureSyndicationProfile::getProfile($update['profile']);
        if ($profile) {
          if ($profile->version() == $update['version']) {
            $profile->update($update['uuid'], $update['fields']);
            $success[] = $update['uuid'];
          }
        }
      }
    }
    return $success;
  }

  public function syndicateNextBatch($batch_size = 25) {
    if (empty($this->syndication_queue)) {
      $this->syndication_queue = UniqueQueue::get('secsyn_syndication');
    }
    $batch = array();
    $items = array();
    for ($k = 0; $k < $batch_size; $k++) {
      $item = $this->syndication_queue->claimItem(60);
      if (!empty($item)) {
        $items[] = $item;
        if (!isset($batch[$item->data['remote_server']])) {
          $batch[$item->data['remote_server']] = array();
        }
        $profile = SecureSyndicationProfile::getProfile($item->data['profile']);
        $batch[$item->data['remote_server']][] = array(
          'profile' => $item->data['profile'],
          'version' => $profile->version(),
          'uuid' => $item->data['uuid'],
          'fields' => $profile->package($profile->lookup($item->data['uuid'])),
        );
      } else {
        $k = $batch_size;
      }
    }
    if (empty($items)) {
      return NULL;
    }
    foreach ($batch as $remote_server => $update_list) {
      module_load_include('inc', 'secsyn', 'services/secsyn.sync');
      secsyn_service_invoke_sync($remote_server, $update_list);
    }
    foreach ($items as $item) {
      $this->syndication_queue->deleteItem($item);
    }
    return TRUE;
  }

}