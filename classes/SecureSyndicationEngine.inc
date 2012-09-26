<?php

class SecureSyndicationEngine {

  private $waiting_queue = NULL;
  private $check_queue = NULL;
  private $syndication_queue = NULL;
  private $uuids_checked = array();

  public function __construct() {}

  public function waitingQueueSize() {
    if (empty($this->waiting_queue)) {
      $this->waiting_queue = UniqueQueue::get('secsyn_wait');
    }
    return $this->waiting_queue->itemsLeft();
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

  private function log($message, $variables = array(), $type = WATCHDOG_DEBUG) {
    watchdog('secsyn', $message, $variables, $type);
  }

  public function queue($remote_server, $synprofile_name, $object, $urgent = FALSE) {
    $this->log('SSOQ: queuing object to send to [@remote] using [@profile]', array(
      '@remote' => $remote_server,
      '@profile' => $synprofile_name
    ));
    $profile = SecureSyndicationProfile::getProfile($synprofile_name);
    if (empty($profile)) {
      $this->log('SSOQ: unable to queue object, invalid profile [@profile]', array(
        '@profile' => $synprofile_name,
      ), WATCHDOG_ERROR);
      return FALSE;
    }
    $uuid = $profile->uuid($object);
    if (empty($uuid)) {
      $this->log('SSOQ: unable to queue object, profile could not generate UUID', array(), WATCHDOG_ERROR);
      return FALSE;
    }
    $data = array(
      'remote_server' => $remote_server,
      'profile' => $synprofile_name,
      'uuid' => $uuid,
    );
    $tag = serialize($data);
    $data['urgent'] = $urgent;
    if (empty($this->waiting_queue)) {
      $this->waiting_queue = UniqueQueue::get('secsyn_wait');
    }
    $result = $this->waiting_queue->createItem($data, $tag, $urgent ? 1 : 0);
    if ($result) {
      $this->log('SSOQ: object [@uuid] queued to send to [@remote] using [@profile]', array(
        '@uuid' => $uuid,
        '@remote' => $remote_server,
        '@profile' => $synprofile_name,
      ));
    }
    return $result;
  }

  public function checkQueue($urgent_only = FALSE, $cap = 50) {
    $this->log('SSOQ: starting to process object queue');
    $processed = 0;
    if (empty($this->waiting_queue)) {
      $this->waiting_queue = UniqueQueue::get('secsyn_wait');
    }
    while ($processed < $cap) {
      $item = $this->waiting_queue->claimItem(60, $urgent_only ? 1 : 0);
      if (empty($item)) {
        return FALSE;
      }
      $this->queueForValidation(
          $item->data['remote_server'],
          $item->data['profile'],
          $item->data['uuid'],
          $item->data['urgent']);
      $this->waiting_queue->deleteItem($item);
      $processed++;
    }
    return TRUE;
  }

  public function queueForValidation($remote_server, $synprofile_name, $uuid, $urgent = FALSE) {
    $profile = SecureSyndicationProfile::getProfile($synprofile_name);
    $key = $synprofile_name . '/' . $uuid;
    if (!in_array($key, $this->uuids_checked)) {
      $this->log('SSVQ: Queuing object [@uuid] for verification before being sent to [@remote] using [@profile]', array(
        '@uuid' => $uuid,
        '@remote' => $remote_server,
        '@profile' => $synprofile_name,
      ));
      $this->uuids_checked[] = $key;
      $dependencies = $profile->dependencies($profile->lookup($uuid));
      $this->log('SSVQ: Examining object [@uuid] for dependencies', array(
        '@uuid' => @uuid,
      ));
      foreach ($dependencies as $dependency) {
        $dep_profile = SecureSyndicationProfile::getProfile($dependency['profile']);
        $dep_uuid = $dep_profile->uuid($dependency['object']);
        if (!$this->queueForValidation($remote_server, $dependency['profile'], $dep_uuid, $urgent)) {
          $this->log('SSVQ: Could not queue dependency [@uuid] with profile [@profile]', array(
            '@uuid' => $dep_uuid,
            '@profile' => $dependency['profile'],
          ), WATCHDOG_ERROR);
          return FALSE;
        }
      }
      $data = array(
        'unique_id' => $uuid,
        'remote_server' => $remote_server,
        'profile' => $synprofile_name,
      );
      $hash = serialize($data);
      $data['urgent'] = $urgent;
      if (empty($this->check_queue)) {
        $this->check_queue = UniqueQueue::get('secsyn_check');
      }
      if ($this->check_queue->createItem($data, $hash, $urgent ? 1 : 0)) {
        $this->log('SSVQ: Object [@uuid] queued for verification', array(
          '@uuid' => $uuid,
        ));
        return TRUE;
      }
      return FALSE;
    }
    return TRUE;
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
                $this->log('SSVC: Object [@uuid] queued, different hashes', array(
                  '@uuid' => $update['uuid'],
                ));
              }
              else {
                $this->log('SSVC: Object [@uuid] not queued, identical hashes', array(
                  '@uuid' => $update['uuid'],
                ));
              }
            }
            // if the object doesn't exist, we want to update it all the time
            else {
              $update_uuids[] = $update['uuid'];
              $this->log('SSVC: Object [@uuid] queued, does not exist', array(
                '@uuid' => $update['uuid'],
              ));
            }
          } else {
            $this->log('SSVC: Profile version does not match remote profile [@profile] vs [@remote_version]', array(
              '@profile' => $update['profile'] . '-' . $profile->version(),
              '@remote_version' => $update['profile'] . '-' . $update['version'],
            ), WATCHDOG_ERROR);
          }
        } else {
          $this->log('SSVC: Profile could not be instanstiated [@profile]', array(
            '@profile' => $update['profile'],
          ), WATCHDOG_ERROR);
        }
      } else {
        $this->log('SSVC: Profile does not exist [@profile]', array(
          '@profile' => $update['profile'],
        ), WATCHDOG_ERROR);
      }
    }
    // this should now be a list of UUIDs that need updating on this server
    return $update_uuids;
  }

  public function checkNextBatch($only_urgent = FALSE, $batch_size = 250) {
    $this->log('SSVQ: starting to process verification queue');
    if (empty($this->check_queue)) {
      $this->check_queue = UniqueQueue::get('secsyn_check');
    }
    $batch = array();
    $original_items = array();
    for ($k = 0; $k < $batch_size; $k++) {
      $nextItem = $this->check_queue->claimItem(60, $only_urgent ? 1 : 0);
      if (!empty($nextItem)) {
        $this->log('SSVQ: adding item [@uuid] to batch send to [@remote] using [@profile]', array(
          '@uuid' => $nextItem->data['unique_id'],
          '@remote' => $nextItem->data['remote_server'],
          '@profile' => $nextItem->data['profile'],
        ));
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
          'urgent' => $nextItem->data['urgent'],
        );
      } else {
        $k = $batch_size;
      }
    }
    $this->log('SSVQ: [@count] item(s) queued in batches', array(
      '@count' => count($original_items),
    ));
    if (!count($original_items)) {
      return NULL;
    }
    foreach ($batch as $remote_server => $items) {
      $this->log('SSVQ: sending batch to [@remote] with [@count] item(s)', array(
        '@remote' => $remote_server,
        '@count' => count($items),
      ));
      module_load_include('inc', 'secsyn', 'services/secsyn.check');
      $results = secsyn_service_invoke_check($remote_server, $items);
      $this->log('SSVQ: received results from [@remote]', array(
        '@remote' => $remote_server,
      ));
      foreach ($results as $result_id) {
        $this->queueForSyndication($remote_server, $items[$result_id]['profile'], $items[$result_id]['uuid'], $items[$result_id]['urgent']);
      }
    }
    foreach ($original_items as $item) {
      $this->check_queue->deleteItem($item);
    }
    $this->log('SSVQ: verification complete');
    return TRUE;
  }

  public function queueForSyndication($remote_server, $profile, $uuid, $urgent = FALSE) {
    $this->log('SSSQ: queuing item [@uuid] for syndication to [@remote] using [@profile]', array(
      '@uuid' => $uuid,
      '@remote' => $remote_server,
      '@profile' => $profile,
    ));
    $data = array(
      'remote_server' => $remote_server,
      'profile' => $profile,
      'uuid' => $uuid,
    );
    if (empty($this->syndication_queue)) {
      $this->syndication_queue = UniqueQueue::get('secsyn_syndication');
    }
    if ($this->syndication_queue->createItem($data, serialize($data), $urgent ? 1 : 0)) {
      $this->log('SSSQ: item [@uuid] successfully queued');
      return TRUE;
    }
    return FALSE;
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
          } else {
            $this->log('SSSQ: profile [@profile] versions do not match [renote:@v1] vs [local:@v2]', array(
              '@profile' => $update['profile'],
              '@v1' => $update['version'],
              '@v2' => $profile->version(),
            ), WATCHDOG_ERROR);
          }
        } else {
          $this->log('SSSQ: profile [@profile] could not be instanstiated', array(
            '@profile' => $update['profile'],
          ), WATCHDOG_ERROR);
        }
      } else {
        $this->log('SSSQ: profile [@profile] not found', array(
          '@profile' => $update['profile'],
        ), WATCHDOG_ERROR);
      }
    }
    return $success;
  }

  public function syndicateNextBatch($only_urgent = FALSE, $batch_size = 25) {
    $this->log('SSSQ: beginning to process queue');
    if (empty($this->syndication_queue)) {
      $this->syndication_queue = UniqueQueue::get('secsyn_syndication');
    }
    $batch = array();
    $items = array();
    for ($k = 0; $k < $batch_size; $k++) {
      $item = $this->syndication_queue->claimItem(60, $only_urgent ? 1 : 0);
      if (!empty($item)) {
        $this->log('SSSQ: adding item [@uuid] to batch send for [@remote]', array(
          '@uuid' => $item->data['uuid'],
          '@remote' => $item->data['remote_server'],
        ));
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
    $this->log('SSSQ: done adding items to batch sends');
    if (empty($items)) {
      return NULL;
    }
    foreach ($batch as $remote_server => $update_list) {
      $this->log('SSSQ: sending batch to [@remote] to update [@count] item(s)', array(
        '@remote' => $remote_server,
        '@count' => count($update_list),
      ));
      module_load_include('inc', 'secsyn', 'services/secsyn.sync');
      secsyn_service_invoke_sync($remote_server, $update_list);
    }
    foreach ($items as $item) {
      $this->syndication_queue->deleteItem($item);
    }
    $this->log('SSSQ: done processing');
    return TRUE;
  }

}