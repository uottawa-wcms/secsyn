<?php

function secsyn_pull_service_get_filter_list() {
  $list = array();
  $rs = db_query('SELECT * FROM {secsyn_filters_profile} ORDER BY title ASC');
  foreach ($rs as $row) {
    $list[] = array(
      'title' => $row->title,
      'profile_name' => $row->machine_name,
    );
  }
  return $list;
}

function secsyn_pull_service_request_filter_list($remote) {
  try {
    $service = new TwoWayService('secsyn_pull', 'filter_list', $remote);
    return $service->invoke();
  } catch (TwoWayException $e) {
    $e->dsm();
    $e->watchdog();
  }
  return array();
}