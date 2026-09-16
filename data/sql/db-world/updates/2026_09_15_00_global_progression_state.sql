CREATE TABLE IF NOT EXISTS `phase_progression_state` (
    `id` TINYINT UNSIGNED NOT NULL,
    `active_phase` TINYINT UNSIGNED NOT NULL DEFAULT 20,
    `active_content_stage` TINYINT UNSIGNED NOT NULL DEFAULT 0,
    `updated_at` TIMESTAMP NOT NULL
        DEFAULT CURRENT_TIMESTAMP
        ON UPDATE CURRENT_TIMESTAMP,
    PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

SET @gpa_has_content_stage = (
    SELECT COUNT(*)
    FROM INFORMATION_SCHEMA.COLUMNS
    WHERE TABLE_SCHEMA = DATABASE()
      AND TABLE_NAME = 'phase_progression_state'
      AND COLUMN_NAME = 'active_content_stage'
);

SET @gpa_schema_sql = IF(
    @gpa_has_content_stage = 0,
    'ALTER TABLE `phase_progression_state`
        ADD COLUMN `active_content_stage`
        TINYINT UNSIGNED NOT NULL DEFAULT 0
        AFTER `active_phase`',
    'SELECT 1'
);

PREPARE gpa_schema_stmt FROM @gpa_schema_sql;
EXECUTE gpa_schema_stmt;
DEALLOCATE PREPARE gpa_schema_stmt;
