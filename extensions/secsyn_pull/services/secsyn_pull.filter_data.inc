<?php

function secsyn_pull_service_get_filter_data($args) {
  $filter = secsyn_filters_profile_load($args['filter_name']);
  if ($filter) {
    return secsyn_filters_get_content($filter);
  }
  return array();
}

function secsyn_pull_service_request_filter_data($remote, $remote_filter_name) {
  try {
    $service = new TwoWayService('secsyn_pull', 'filter_data', $remote);
    return $service->invoke(array(
      'filter_name' => $remote_filter_name,
    ));
  } catch (TwoWayException $e) {
    $e->dsm();
    $e->watchdog();
  }
  return array();
}