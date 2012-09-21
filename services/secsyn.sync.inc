<?php

  function secsyn_service_invoke_sync($remote_server, $check_data) {
    $service = new TwoWayService('secsyn', 'sync', $remote_server);
    try {
      $service->invoke(array(
        'updates' => $check_data
      ));
      return TRUE;
    } catch (TwoWayException $e) {
      $e->dsm();
      $e->watchdog('secsyn', 'An error occured while synchronizing content');
    }
    return FALSE;
  }

  function secsyn_service_do_sync($args) {
    $args = (array) $args;
    $engine = new SecureSyndicationEngine();
    return $engine->executeUpdates((array) $args['updates']);
  }