<?php

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
