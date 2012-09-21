<?php

function secsyn_syndicate_all($from_cron = FALSE) {
  $start_time = time();
  $elapsed_time = 0;
  $max_time = 15;
  $mode = 0;
  $modes = array('checkNextBatch', 'syndicateNextBatch');
  while (($mode < count($modes)) && ($elapsed_time < $max_time)) {
    $engine = new SecureSyndicationEngine();
    $current_operation = $modes[$mode];
    $result = $engine->$current_operation();
    if (empty($result)) {
      $mode++;
    }
    $elapsed_time = time() - $start_time;
  }
  if (!$from_cron) {
    return '<p>Syndication Process Complete</p>';
  }
}

function secsyn_status_page() {
  $engine = new SecureSyndicationEngine();
  $content = '<p>';
  $content .= '<strong>' . t('Queued for validation: @total', array('@total' => $engine->validationQueueSize())) . '</strong><br />';
  $content .= '<strong>' . t('Queued for syndication: @total', array('@total' => $engine->syndicationQueueSize())) . '</strong><br />';
  $content .= '</p>';
  return $content;
}