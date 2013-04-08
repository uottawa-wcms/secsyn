<?php

/**
 * Page callback for forcing a cron
 */
function secsyn_force_page() {
  // @todo we probably want to move this to some kind of settings page
  module_load_include('inc', 'secsyn', 'secsyn.functions');
  secsyn_syndicate_all(FALSE);
  return '<p>' . t('Syndication complete') . '</p>';
}

/**
 * Page callback for the main status page.
 */
function secsyn_status_page() {
  // @todo: we want better formatting for this
  $engine = new SecureSyndicationEngine();
  $content = '<p>';
  $content .= '<strong>' . t('Waiting for syndication: @total', array('@total' => $engine->waitingQueueSize())) . '</strong><br />';
  $content .= '<strong>' . t('Queued for validation: @total', array('@total' => $engine->validationQueueSize())) . '</strong><br />';
  $content .= '<strong>' . t('Queued for syndication: @total', array('@total' => $engine->syndicationQueueSize())) . '</strong><br />';
  $content .= '</p>';
  return $content;
}

function secsyn_server_admin_form($form, &$form_state) {
  $servers = twoway_server_list();
  $header = array(
    'server' => 'Server Name',
  );
  $rows = array();
  foreach ($servers as $mname => $server) {
    $rows[$mname] = array(
      'server' => $server,
    );
  }
  $form['message'] = array(
    '#type' => 'markup',
    '#markup' => '<p>' . t('Select the servers you want to make available for
        persistent connections.') . '</p>',
  );
  $form['server_list'] = array(
    '#type' => 'tableselect',
    '#header' => $header,
    '#options' => $rows,
    '#default_value' => variable_get('secsyn_links_servers', array()),
    '#empty' => t('There are no servers registered with Two-Way'),
  );
  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Save'),
  );
  return $form;
}

function secsyn_server_admin_form_submit($form, &$form_state) {
  $servers = array();
  foreach ($form_state['values']['server_list'] as $key => $value) {
    if (!empty($value)) {
      $servers[$value] = $value;
    }
  }
  variable_set('secsyn_links_servers', $servers);
  drupal_set_message(t('Content servers updated'));
}
