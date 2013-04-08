<?php

function secsyn_links_admin_form($form, &$form_state) {
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

function secsyn_links_admin_form_submit($form, &$form_state) {
  $servers = array();
  foreach ($form_state['values']['server_list'] as $key => $value) {
    if (!empty($value)) {
      $servers[$value] = $value;
    }
  }
  variable_set('secsyn_links_servers', $servers);
  drupal_set_message(t('Content servers updated'));
}
