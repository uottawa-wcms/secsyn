<?php

define('SECSYN_LINKS_CREATE', 'create');
define('SECSYN_LINKS_DELETE', 'delete');

function secsyn_links_service_invoke_update_link($link, $op = SECSYN_LINKS_CREATE) {
  $destination = $link['remote_server'];
  $service = new TwoWayService('secsyn_links', 'update_link', $destination);
  $link['remote_server'] = twoway_get_server_path();
  try {
    $results = $service->invoke(array(
      'link' => $link,
      'op' => $op,
    ));
    return $results;
  } catch (TwoWayException $e) {
    $e->dsm();
    $e->watchdog('secsyn', 'An error occured while sending a request to create a persistent link');
    return array();
  }
}

function secsyn_links_service_do_update_link($args) {
  $args = (array) $args;
  if ($args['op'] == SECSYN_LINKS_DELETE) {
    return secsyn_links_delete_link($args['link']);
  } else {
    return secsyn_links_create_link($args['link']);
  }
}