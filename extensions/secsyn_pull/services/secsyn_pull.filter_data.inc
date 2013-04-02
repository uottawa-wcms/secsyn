<?php

/**
 * Service callback to retrieve the list of objects matching a filter name.
 *
 * @param array $args Service arguments:
 * - filter_name: The machine name of the filter to load
 * @return array An array of content items to display.
 */
function secsyn_pull_service_get_filter_data($args) {
  $filter = secsyn_filters_profile_load($args['filter_name']);
  if ($filter) {
    return secsyn_filters_get_content($filter);
  }
  return array();
}

/**
 * Creates a service request to get a list of objects matching a remote filter.
 *
 * @param string $remote Remote server identifier
 * @param string $remote_filter_name Machine name of filter set
 * @return array An array of content items to display.
 */
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