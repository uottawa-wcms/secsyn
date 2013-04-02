<?php

/**
 * Processes a request to queue objects to a remote server.
 *
 * @param array $args Service arguments:
 * - filter_name: Queue all items from this filter
 * - is_urgent: If TRUE, queue urgently
 * - requesting_server: URL of remote server to send objects to
 * - object_uuids: UUIDs of objects to queue
 *
 * @return array An array of UUIDs that were queued successfully.
 */
function secsyn_pull_service_do_queue($args) {
  $filter = secsyn_filters_profile_load($args['filter_name']);
  $profile = SecureSyndicationProfile::getProfile($filter->sync_profile_name);
  $urgent = !empty($args['is_urgent']);
  $remote = $args['requesting_server'];
  $engine = new SecureSyndicationEngine();
  $success = array();
  foreach ($args['object_uuids'] as $uuid) {
    $object = $profile->lookup($uuid);
    if ($object) {
      if ($engine->queue($remote, $filter->sync_profile_name, $object, $urgent)) {
        $success[] = $uuid;
      }
    }
  }
  return $success;
}

/**
 * Invokes a call to a remote server to queue objects to send to this server.
 *
 * @param string $remote Remote server to request to queue objects
 * @param string $filter_name Name of the filter we're queuing objects for
 * @param array $object_uuids Array of object UUIDs we're queuing
 * @param boolean $urgent TRUE if we should queue urgently.
 * @return int|FALSE Number of items queued or FALSE on error
 */
function secsyn_pull_service_invoke_queue($remote, $filter_name, array $object_uuids, $urgent = FALSE) {
  try {
    $service = new TwoWayService('secsyn_pull', 'queue', $remote);
    $items = $service->invoke(array(
      'requesting_server' => twoway_get_server_path(),
      'is_urgent' => $urgent,
      'filter_name' => $filter_name,
      'object_uuids' => $object_uuids,
    ));
    return count($items);
  } catch (TwoWayException $e) {
    $e->dsm();
    $e->watchdog();
    return FALSE;
  }
}
