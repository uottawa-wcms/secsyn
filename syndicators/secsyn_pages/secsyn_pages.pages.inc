<?php

function secsyn_pages_secsyn_exportable_children($object) {
  $deps = array();
  if ($object->table == 'page_manager_pages') {
    $handlers = db_select('page_manager_handlers', 'pmh')
        ->condition('pmh.task', 'page')
        ->condition('pmh.subtask', $object->name)
        ->fields('pmh')
        ->execute();
    foreach ($handlers as $handler) {
      $deps[] = array(
        'profile' => 'SecureSyndicationExportableProfile',
        'object' => $handler,
      );
    }
  }
  return $deps;
}

function _secsyn_pages_load_pages_list() {
  $rs = db_select('page_manager_pages', 'pmp')
      ->fields('pmp', array('name', 'admin_title'))
      ->execute();
  $options = array();
  foreach ($rs as $row) {
    $options[$row->name] = $row->admin_title;
  }
  return $options;
}

function secsyn_pages_syndicate_form_halfway($form, &$form_state) {
  $form_state['rebuild'] = TRUE;
}

function secsyn_pages_syndicate_form($form, &$form_state) {
  if (empty($form_state['values']['page'])) {
    $form['page'] = array(
      '#type' => 'select',
      '#title' => t('Page'),
      '#required' => TRUE,
      '#empty_value' => '',
      '#default_value' => '',
      '#empty_option' => t('Select one...'),
      '#options' => _secsyn_pages_load_pages_list(),
    );
    $form['next'] = array(
      '#type' => 'submit',
      '#submit' => array('secsyn_pages_syndicate_form_halfway'),
      '#value' => t('Next'),
    );
  } else {
    $form['page'] = array(
      '#type' => 'value',
      '#value' => $form_state['values']['page'],
    );
    $page = ctools_export_crud_load('page_manager_pages', $form_state['values']['page']);
    _secsyn_common_fields($form, $page, 'SecureSyndicationExportableProfile');
    $form['syndicate'] = array(
      '#type' => 'submit',
      '#value' => t('Syndicate'),
    );
  }
  return $form;
}

function secsyn_pages_syndicate_form_submit($form, &$form_state) {
  ctools_include('export');
  $page = ctools_export_crud_load('page_manager_pages', $form_state['values']['page']);
  _secsyn_common_fields_submit('SecureSyndicationExportableProfile', $page, $form_state);
  drupal_set_message(t('Object queued for syndication'));
  $form_state['redirect'] = 'admin/structure/pages';
}