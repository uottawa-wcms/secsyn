<?php

function secsyn_service_invoke_check($remote_server, $check_data) {
  $service = new TwoWayService('secsyn', 'check', $remote_server);
  try {
    $results = $service->invoke(array(
      'potential_updates' => $check_data
    ));
    return $results;
  } catch (TwoWayException $e) {
    $e->dsm();
    $e->watchdog('secsyn', 'An error occured while verifying which content pieces to synchronize');
    return array();
  }
}

function secsyn_service_do_check($args) {
  $args = (array) $args;
  $engine = new SecureSyndicationEngine();
  return $engine->checkForUpdates((array) $args['potential_updates']);
}