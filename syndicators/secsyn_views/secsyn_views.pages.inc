<?php

function secsyn_views_syndicate_form($form, &$form_state) {
  $items = views_get_all_views();
  $list = array();
  foreach ($items as $key => $item) {
    $list[$key] = $item->human_name;
  }
  $form = array(
    'view' => array(
      '#type' => 'select',
      '#title' => t('View'),
      '#required' => TRUE,
      '#empty_value' => '',
      '#default_value' => '',
      '#empty_option' => t('Select one...'),
      '#options' => $list,
    ),
    'remote_server' => array(
      '#type' => 'select',
      '#title' => t('Remote Server'),
      '#required' => TRUE,
      '#options' => twoway_server_list(),
      '#empty_value' => '',
      '#default_value' => '',
      '#empty_option' => t('Select one...'),
    ),
    'submit' => array(
      '#type' => 'submit',
      '#value' => t('Queue'),
      '#weight' => 10,
    ),
  );
  if (user_access('urgent syndication')) {
    $form['urgent'] = array(
      '#type' => 'checkbox',
      '#title' => t('Urgent syndication'),
      '#weight' => 5,
    );
  }
  return $form;
}

function secsyn_views_syndicate_form_submit($form, &$form_state) {
  ctools_include('export');
  $view = ctools_export_crud_load('views_view', $form_state['values']['view']);
  $remote = twoway_server_load($form_state['values']['remote_server']);
  $engine = new SecureSyndicationEngine();
  if ($engine->queue(
      $remote['server_connection'],
      'SecureSyndicationExportableProfile',
      $view,
      !empty($form_state['values']['urgent'])
  )) {
    drupal_set_message(t('View queued for syndication'));
    $form_state['redirect'] = 'admin/structure/views';
  }
}