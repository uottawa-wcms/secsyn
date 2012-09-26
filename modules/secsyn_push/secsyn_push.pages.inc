<?php

function secsyn_push_form($form, &$form_state) {
  $form = array();
  if (!empty($form_state['values']['filter'])) {
    $form['filter'] = array(
      '#type' => 'value',
      '#value' => $form_state['values']['filter'],
    );
    $form['stage_two'] = array(
      '#type' => 'value',
      '#value' => TRUE,
    );
    $form['remote_server'] = array(
      '#type' => 'select',
      '#options' => twoway_server_list(),
      '#required' => TRUE,
      '#empty_value' => '',
      '#default_value' => '',
      '#empty_option' => 'Select one...',
      '#title' => t('Remote Server'),
    );
    if (user_access('urgent syndication')) {
      $form['urgent'] = array(
        '#type' => 'checkbox',
        '#title' => t('Urgent syndication'),
      );
    }
    $content_list = secsyn_filters_get_content(secsyn_filters_profile_load($form_state['values']['filter']));
    $header = array(
      'content' => t('Content'),
      'preview' => t('Preview'),
    );
    $rows = array();
    foreach ($content_list as $content) {
      $rows[$content['uuid']] = array(
        'content' => l($content['label'], $content['link']),
        'preview' => drupal_render($content['preview']),
      );
    }
    $form['content_select'] = array(
      '#type' => 'tableselect',
      '#header' => $header,
      '#options' => $rows,
      '#empty' => t('No content was found that matches the filter'),
    );
    $form['submit_queue'] = array(
      '#type' => 'submit',
      '#value' => t('Queue Items'),
    );
  } else {
    $filters = array();
    $rs = db_query('SELECT * FROM {secsyn_filters_profile} ORDER BY title');
    foreach ($rs as $row) {
      $filters[$row->machine_name] = $row->title;
    }
    $form['filter'] = array(
      '#type' => 'select',
      '#options' => $filters,
      '#title' => t('Filter'),
      '#required' => TRUE,
      '#empty_value' => '',
      '#default_value' => '',
      '#empty_option' => t('Select one...'),
    );
    $form['load_list'] = array(
      '#type' => 'submit',
      '#value' => t('Load Content'),
    );
  }
  return $form;
}

function secsyn_push_form_submit($form, &$form_state) {
  if (!empty($form_state['values']['stage_two'])) {
    $remote = (array) twoway_server_load($form_state['values']['remote_server']);
    $connection = $remote['server_connection'];
    $urgent = !empty($form_state['values']['urgent']);
    $filter = secsyn_filters_profile_load($form_state['values']['filter']);
    $profile = $filter->sync_profile_name;
    $engine = new SecureSyndicationEngine();
    $profile_object = SecureSyndicationProfile::getProfile($profile);
    $total = 0;
    foreach ($form_state['values']['content_select'] as $key => $val) {
      if ((!empty($val)) && ($val == $key)) {
        if ($engine->queue($connection, $profile, $profile_object->lookup($key), $urgent)) {
          $total++;
        }
      }
    }
    drupal_set_message(t('@count object(s) have been queued for syndication', array(
      '@count' => $total,
    )));
    $form_state['redirect'] = 'admin/content/syndication';
  } else {
    $form_state['rebuild'] = TRUE;
  }
}