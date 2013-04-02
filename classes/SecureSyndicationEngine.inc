<?php

/**
 * This class is responsible for maintaining and processing the syndication
 * queue.
 *
 * There are three queues:
 *
 * * Waiting Queue: items before being checked for dependencies
 * * Validation Queue: items before being validated against the remote server
 * * Syndication Queue: items ready to be syndicated.
 *
 * Generally you will not need to use this class. You should use either
 * _secsyn_fast_queue() to queue items for syndication or secsyn_syndicate_all()
 * to run the syndication process.
 *
 * @todo Add additional event logging so that users can have a clear idea of
 * when the last time any given item was syndicated, using what profile, etc.
 */
class SecureSyndicationEngine {

  /**
   * The wait queue object.
   *
   * This holds objects before they have been checked for dependencies.
   *
   * @var \UniqueQueue
   */
  private $waiting_queue = NULL;

  /**
   * The validation queue object.
   *
   * This holds objects before they have been checked against the remote server
   * that they need to be syndicated.
   *
   * @var \UniqueQueue
   */
  private $check_queue = NULL;

  /**
   * The syndication queue object.
   *
   * This holds objects before they have actually been syndicated.
   *
   * @var \UniqueQueue
   */
  private $syndication_queue = NULL;

  /**
   * A list of unique IDs that have already been processed by this engine
   * object for dependencies. Items in this array will not be reprocessed. This
   * helps us avoid infinite recursion.
   *
   * @var array
   */
  private $uuids_checked = array();

  /**
   * Set to TRUE to add debugging messages to the output.
   * @var boolean
   */
  private $debug = FALSE;

  /**
   * Boring constructor - we lazy load everything we need.
   */
  public function __construct() {}

  /**
   * Checks how many items are in the wait queue.
   *
   * @return int Number of items in the wait queue.
   */
  public function waitingQueueSize() {
    if (empty($this->waiting_queue)) {
      $this->waiting_queue = UniqueQueue::get('secsyn_wait');
    }
    return $this->waiting_queue->itemsLeft();
  }

  /**
   * Checks how many items are in the validation queue.
   *
   * @return int Number of items in the validation queue.
   */
  public function validationQueueSize() {
    if (empty($this->check_queue)) {
      $this->check_queue = UniqueQueue::get('secsyn_check');
    }
    return $this->check_queue->itemsLeft();
  }

  /**
   * Checks how many items are in the syndication queue.
   *
   * @return int Number of the items in the syndication queue.
   */
  public function syndicationQueueSize() {
    if (empty($this->syndication_queue)) {
      $this->syndication_queue = UniqueQueue::get('secsyn_syndication');
    }
    return $this->syndication_queue->itemsLeft();
  }

  /**
   * Logs a message.
   *
   * This adds a message to watchdog() and, if debug is on, also does a dsm().
   *
   * @param string $message The message to log
   * @param array $variables Variables for the log message
   * @param int $type One of the WATCHDOG_X types
   */
  private function log($message, $variables = array(), $type = WATCHDOG_DEBUG) {
    watchdog('secsyn', $message, $variables, $type);
    if ($this->debug) {
      if ($type == WATCHDOG_DEBUG || $type == WATCHDOG_INFO) {
        drupal_set_message(t($message, $variables), 'debug');
      } else {
        drupal_set_message(t($message, $variables), 'error');
      }
    }
  }

  /**
   * Adds an item to the wait queue.
   *
   * This is the primary method that should be called for new items.
   *
   * @param string $remote_server The machine name of a TwoWay server OR the
   * URL path to a Drupal installation
   * @param string $synprofile_name The name of a syndication profile class
   * @param mixed $object The object to syndicate, as appropriate for the profile
   * @param boolean $urgent TRUE to make this an urgent syndication
   *
   * @return TRUE if the object was added to the queue, otherwise FALSE
   */
  public function queue($remote_server, $synprofile_name, $object, $urgent = FALSE) {
    $this->log('Adding object to wait queue (target: [@remote] using [@profile])', array(
      '@remote' => $remote_server,
      '@profile' => $synprofile_name
    ));
    $profile = SecureSyndicationProfile::getProfile($synprofile_name);
    if (empty($profile)) {
      $this->log('Unable to add object to wait queue, invalid profile [@profile]', array(
        '@profile' => $synprofile_name,
      ), WATCHDOG_ERROR);
      return FALSE;
    }
    $uuid = $profile->uuid($object);
    if (empty($uuid)) {
      $this->log('Unable to add object to wait queue, profile could not generate UUID [@profile]', array(
        '@profile' => $synprofile_name,
      ), WATCHDOG_ERROR);
      return FALSE;
    }
    $data = array(
      'remote_server' => $remote_server,
      'profile' => $synprofile_name,
      'uuid' => $uuid,
    );
    // create a unique hash based on $data
    $tag = serialize($data);
    $data['urgent'] = $urgent;
    // create instance of queue
    if (empty($this->waiting_queue)) {
      $this->waiting_queue = UniqueQueue::get('secsyn_wait');
    }
    // @todo: pre-queue hook
    $result = $this->waiting_queue->createItem($data, $tag, $urgent ? 1 : 0);
    // @todo: post-queue hook
    if ($result) {
      $this->log('Wait queue accepted object [@uuid] to send to [@remote] using [@profile]', array(
        '@uuid' => $uuid,
        '@remote' => $remote_server,
        '@profile' => $synprofile_name,
      ));
    }
    return $result;
  }

  /**
   * Processes items out of the wait queue.
   *
   * @param boolean $urgent_only If TRUE, only urgent items are used.
   * @param int $cap Maximum number of items to syndicate
   * @return TRUE if there are more items, otherwise FALSE
   */
  public function checkQueue($urgent_only = FALSE, $cap = 50) {
    $this->log('Processing wait queue');
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
    return $this->waiting_queue->itemsLeft() > 0;
  }

  /**
   * Helper function that handles queuing dependencies from an object.
   *
   * Dependent objects are those that are essential parts of our current object
   * but must be created before ours.
   *
   * @param string $remote_server The remote server we're referencing.
   * @param array $dependencies An array of dependencies. Each dependency should
   *   be an associative array with two keys:
   *   * profile: The class name of the profile to use
   *   * object: The object as needed for the profile.
   * @param boolean $urgent Whether or not the queue is urgent
   * @return TRUE if all dependencies were added fine or FALSE if one or
   *   more failed.
   */
  private function queueDependencies($remote_server, $dependencies, $urgent = FALSE) {
    foreach ($dependencies as $dependency) {
      $dep_profile = SecureSyndicationProfile::getProfile($dependency['profile']);
      if (empty($dep_profile)) {
        $this->log('Could not load profile for a dependency [@profile]', array(
          '@profile' => $dependency['profile'],
        ), WATCHDOG_ERROR);
        return FALSE;
      }
      $dep_uuid = $dep_profile->uuid($dependency['object']);
      if (!$this->queueForValidation($remote_server, $dependency['profile'], $dep_uuid, $urgent)) {
        $this->log('Could not queue dependency [@uuid] with profile [@profile]', array(
          '@uuid' => $dep_uuid,
          '@profile' => $dependency['profile'],
        ), WATCHDOG_ERROR);
        return FALSE;
      }
    }
    return TRUE;
  }

  /**
   * Queues a set of children objects.
   *
   * Child objects are those that are essential parts of our current object
   * but must be created afterwards.
   *
   * @param string $remote_server The remote server we're referencing.
   * @param array $children An array of children. Each child should
   *   be an associative array with two keys:
   *   * profile: The class name of the profile to use
   *   * object: The object as needed for the profile.
   * @param boolean $urgent Whether or not the queue is urgent
   * @return TRUE if all dependencies were added fine or FALSE if one or
   *   more failed.
   */
  private function queueChildren($remote_server, $children, $urgent = FALSE) {
    foreach ($children as $child) {
      $profile = SecureSyndicationProfile::getProfile($child['profile']);
      if (empty($profile)) {
        $this->log('Could not load profile for child [@profile]', array(
          '@profile' => $child['profile'],
        ), WATCHDOG_ERROR);
        return FALSE;
      }
      $uuid = $profile->uuid($child['object']);
      if (!$this->queueForValidation($remote_server, $child['profile'], $uuid, $urgent)) {
        $this->log('Could not queue child object [@uuid] with profile [@profile]', array(
          '@uuid' => $uuid,
          '@profile' => $dependency['profile'],
        ), WATCHDOG_ERROR);
        return FALSE;
      }
    }
    return TRUE;
  }

  /**
   * Queues an object and it's dependencies and children into the validation
   * queue.
   *
   * @param string $remote_server Remote server reference
   * @param string $synprofile_name Class name of the syndication profile
   * @param string $uuid Unique identifier for the object
   * @param boolean $urgent TRUE to queue in an urgent fashion, otherwise FALSE
   * @return TRUE if the object was queued, otherwise FALSE.
   */
  private function queueForValidation($remote_server, $synprofile_name, $uuid, $urgent = FALSE) {
    if (empty($uuid)) {
      $this->log('Could not queue object for validation, empty UUID');
      return FALSE;
    }
    $profile = SecureSyndicationProfile::getProfile($synprofile_name);
    $key = $synprofile_name . '/' . $uuid;
    if (!in_array($key, $this->uuids_checked)) {
      $this->log('Queuing object [@uuid] for verification before being sent to [@remote] using [@profile]', array(
        '@uuid' => $uuid,
        '@remote' => $remote_server,
        '@profile' => $synprofile_name,
      ));
      $this->uuids_checked[] = $key;
      $this->log('Examining object [@uuid] for dependencies', array(
        '@uuid' => $uuid,
      ));
      $syn_object = $profile->lookup($uuid);
      if (!$this->queueDependencies(
          $remote_server,
          $profile->dependencies($syn_object),
          $urgent)) {
        return FALSE;
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
        $this->log('Object [@uuid] queued for verification', array(
          '@uuid' => $uuid,
        ));
        $this->log('Examining object [@uuid] for children', array(
          '@uuid' => $uuid,
        ));
        $results = $this->queueChildren(
            $remote_server,
            $profile->children($syn_object),
            $urgent);
        $this->log('Object [@uuid] completely queued for verification', array(
          '@uuid' => $uuid,
        ));
        return $results;
      }
      return FALSE;
    }
    return TRUE;
  }

  /**
   * This function is called by a remote server with a list of objects to
   * verify.
   *
   * @param array $proposed_updates An array of updates to check:
   * - profile: The syndication profile to use
   * - version: The version number of the remote profile
   * - uuid: The UUID of the object
   * - lookup_fields: The list of fields used to make the object
   * - digest: A string representation of all the field data
   *
   * @return array An array of UUIDs that need updating.
   */
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
                $this->log('Object [@uuid] queued, different hashes', array(
                  '@uuid' => $update['uuid'],
                ));
              }
              else {
                $this->log('Object [@uuid] not queued, identical hashes', array(
                  '@uuid' => $update['uuid'],
                ));
              }
            }
            // if the object doesn't exist, we want to update it all the time
            else {
              $update_uuids[] = $update['uuid'];
              $this->log('Object [@uuid] queued, does not exist', array(
                '@uuid' => $update['uuid'],
              ));
            }
          } else {
            $this->log('Profile version does not match remote profile [@profile] vs [@remote_version]', array(
              '@profile' => $update['profile'] . '-' . $profile->version(),
              '@remote_version' => $update['profile'] . '-' . $update['version'],
            ), WATCHDOG_ERROR);
          }
        } else {
          $this->log('Profile could not be instanstiated [@profile]', array(
            '@profile' => $update['profile'],
          ), WATCHDOG_ERROR);
        }
      } else {
        $this->log('Profile does not exist [@profile]', array(
          '@profile' => $update['profile'],
        ), WATCHDOG_ERROR);
      }
    }
    // this should now be a list of UUIDs that need updating on this server
    return $update_uuids;
  }

  /**
   * Triggers the validation queue.
   *
   * When run, this will process items in batches to be sent to the remote
   * server to verify they have changed and need to be updated.
   *
   * @param boolean $only_urgent Only send urgent objects
   * @param int $batch_size Maximum number of items to send
   * @return TRUE if there are more items to process, otherwise FALSE
   */
  public function checkNextBatch($only_urgent = FALSE, $batch_size = 250) {
    $this->log('Starting to process verification queue');
    if (empty($this->check_queue)) {
      $this->check_queue = UniqueQueue::get('secsyn_check');
    }
    $batch = array();
    $original_items = array();
    for ($k = 0; $k < $batch_size; $k++) {
      $nextItem = $this->check_queue->claimItem(60, $only_urgent ? 1 : 0);
      if (!empty($nextItem)) {
        $this->log('Adding item [@uuid] to batch send to [@remote] using [@profile]', array(
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
        // we have to group them by server to send out
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
    $this->log('[@count] item(s) queued in batches for verification', array(
      '@count' => count($original_items),
    ));
    if (!count($original_items)) {
      return FALSE;
    }
    foreach ($batch as $remote_server => $items) {
      $this->log('Sending batch to [@remote] with [@count] item(s)', array(
        '@remote' => $remote_server,
        '@count' => count($items),
      ));
      module_load_include('inc', 'secsyn', 'services/secsyn.check');
      $results = secsyn_service_invoke_check($remote_server, $items);
      $this->log('Received results of verification from [@remote]', array(
        '@remote' => $remote_server,
      ));
      foreach ($results as $result_id) {
        $this->queueForSyndication($remote_server, $items[$result_id]['profile'], $items[$result_id]['uuid'], $items[$result_id]['urgent']);
      }
    }
    foreach ($original_items as $item) {
      $this->check_queue->deleteItem($item);
    }
    $this->log('Verification complete');
    return TRUE;
  }

  /**
   * Adds an item to the syndication queue.
   *
   * @param string $remote_server The remote server reference
   * @param string $profile The classname of the profile
   * @param string $uuid The unique identifier
   * @param boolean $urgent TRUE if the object is urgent
   * @return boolean TRUE if the object was queued, otherwise false
   */
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

  /**
   * Processes an update request from a remote server.
   *
   * @param string $updates An array of updates:
   * - profile: The profile classname
   * - version: The profile version
   * - uuid: The UUID of the object
   * - fields: The data to update
   * @return type
   */
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
            $this->log('Profile [@profile] versions do not match [remote:@v1] vs [local:@v2]', array(
              '@profile' => $update['profile'],
              '@v1' => $update['version'],
              '@v2' => $profile->version(),
            ), WATCHDOG_ERROR);
          }
        } else {
          $this->log('profile [@profile] could not be instanstiated', array(
            '@profile' => $update['profile'],
          ), WATCHDOG_ERROR);
        }
      } else {
        $this->log('Profile [@profile] not found', array(
          '@profile' => $update['profile'],
        ), WATCHDOG_ERROR);
      }
    }
    return $success;
  }

  /**
   * Syndicates objects to remote server.
   *
   * @param boolean $only_urgent TRUE if we should syndicate only urgent objects.
   * @param int $batch_size The number of items to send
   * @return TRUE if there are more items to process
   */
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
        $this->log('SSSQ: adding item [@uuid] to batch send for [@remote] using [@profile]', array(
          '@uuid' => $item->data['uuid'],
          '@remote' => $item->data['remote_server'],
          '@profile' => $item->data['profile'],
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
      return FALSE;
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